// filetime -- display and manipulate file timestamps with sub-second precision
// Copyright (C) 2026 Free Software Foundation, Inc.
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.

use anyhow::{bail, Context, Result};
use clap::{ArgGroup, Parser};
use std::ffi::CString;
use std::os::unix::fs::MetadataExt;
use std::path::Path;

// ─── CLI definition ──────────────────────────────────────────────────────────

#[derive(Parser)]
#[command(
    name = "filetime",
    version,
    about = "Display or manipulate file timestamps with sub-second precision.\n\n\
             Without an action option, displays all timestamps of each FILE.\n\
             Selection flags (-a/-m/-c/-b) filter which timestamps are shown\n\
             or, for write actions, which writable timestamps are changed.\n\n\
             Readable timestamps (always):\n  \
               access   (-a)  last read\n  \
               modify   (-m)  last content write\n  \
               change   (-c)  last metadata change  [kernel-managed, read-only]\n  \
               birth    (-b)  creation time         [kernel-managed, read-only]\n\n\
             TIME formats:\n  \
               @SECONDS[.FRAC]               Unix epoch seconds\n  \
               'YYYY-MM-DD HH:MM:SS[.FRAC]'  local date-time\n\n\
             DELTA format:  [+-]SECONDS[.FRAC]  e.g. +1.5, -0.000100"
)]
#[command(group(ArgGroup::new("action").args(["set", "copy_from", "adjust"])))]
struct Cli {
    /// Show/act on access time
    #[arg(short = 'a', long)]
    atime: bool,

    /// Show/act on modification time
    #[arg(short = 'm', long)]
    mtime: bool,

    /// Show change (metadata) time — read-only, kernel-managed
    #[arg(short = 'c', long)]
    ctime: bool,

    /// Show birth (creation) time — read-only, may be unavailable on some
    /// filesystems
    #[arg(short = 'b', long)]
    btime: bool,

    /// Set timestamps to TIME
    #[arg(short = 's', long, value_name = "TIME")]
    set: Option<String>,

    /// Replicate timestamps from FILE (preserves full nanosecond precision)
    #[arg(long = "copy-from", value_name = "FILE")]
    copy_from: Option<String>,

    /// Shift existing timestamps by DELTA seconds
    #[arg(long, value_name = "DELTA", allow_hyphen_values = true)]
    adjust: Option<String>,

    /// Affect symbolic links themselves instead of their targets
    #[arg(short = 'n', long = "no-dereference")]
    no_dereference: bool,

    /// Display timestamps as epoch seconds
    #[arg(short = 'e', long)]
    epoch: bool,

    /// Fractional-second digits to display: 0, 3, 6 (default), or 9
    #[arg(short = 'p', long, value_name = "N", default_value = "6")]
    precision: u32,

    /// Files to operate on
    #[arg(value_name = "FILE", required = true)]
    files: Vec<String>,
}

// ─── Timestamp type ───────────────────────────────────────────────────────────

/// A timestamp as (seconds, nanoseconds) since the Unix epoch.
#[derive(Clone, Copy, Debug)]
struct Ts {
    sec: i64,
    nsec: i64,
}

impl Ts {
    fn now() -> Self {
        let d = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap_or_default();
        Ts { sec: d.as_secs() as i64, nsec: d.subsec_nanos() as i64 }
    }

    fn add(self, delta: Ts, negative: bool) -> Ts {
        if !negative {
            let mut nsec = self.nsec + delta.nsec;
            let mut sec = self.sec + delta.sec;
            if nsec >= 1_000_000_000 { sec += 1; nsec -= 1_000_000_000; }
            Ts { sec, nsec }
        } else {
            let mut nsec = self.nsec - delta.nsec;
            let mut sec = self.sec - delta.sec;
            if nsec < 0 { sec -= 1; nsec += 1_000_000_000; }
            Ts { sec, nsec }
        }
    }
}

// ─── All timestamps for one file ─────────────────────────────────────────────

/// Every timestamp associated with a file.
struct FileTimes {
    /// Last access (read).
    atime: Ts,
    /// Last data modification (write).
    mtime: Ts,
    /// Last metadata change (chmod, chown, link, rename, …).
    /// Always present; cannot be set via any user-space API.
    ctime: Ts,
    /// File creation / birth time.
    /// `None` when the filesystem or kernel does not report it.
    btime: Option<Ts>,
}

/// Read all four timestamps via `statx(2)`.
///
/// `statx` is used (rather than plain `stat`) because it is the only POSIX-ish
/// syscall on Linux that exposes the birth time (`stx_btime`).
fn get_all_times(path: &str, no_deref: bool) -> Result<FileTimes> {
    let cpath = CString::new(path).with_context(|| format!("invalid path '{path}'"))?;
    let flags = if no_deref { libc::AT_SYMLINK_NOFOLLOW } else { 0 };
    let mask  = libc::STATX_ATIME | libc::STATX_MTIME
              | libc::STATX_CTIME | libc::STATX_BTIME;

    // SAFETY: statx writes into the zeroed buffer; all fields are plain data.
    let mut stx: libc::statx = unsafe { std::mem::zeroed() };
    let ret = unsafe {
        libc::statx(libc::AT_FDCWD, cpath.as_ptr(), flags, mask, &mut stx)
    };
    if ret != 0 {
        bail!("cannot stat '{}': {}", path, std::io::Error::last_os_error());
    }

    let from = |t: libc::statx_timestamp| Ts { sec: t.tv_sec, nsec: t.tv_nsec as i64 };

    // Birth time is only valid when the kernel sets STATX_BTIME in stx_mask.
    let btime = if stx.stx_mask & libc::STATX_BTIME != 0 {
        Some(from(stx.stx_btime))
    } else {
        None
    };

    Ok(FileTimes {
        atime: from(stx.stx_atime),
        mtime: from(stx.stx_mtime),
        ctime: from(stx.stx_ctime),
        btime,
    })
}

// ─── Parsing ─────────────────────────────────────────────────────────────────

fn parse_frac(s: &str) -> i64 {
    let digits: String = s.chars().take(9).collect();
    let len = digits.len();
    let val: i64 = digits.parse().unwrap_or(0);
    val * 10i64.pow((9 - len) as u32)
}

fn split_frac(s: &str) -> (&str, i64) {
    match s.find('.') {
        Some(dot) => (&s[..dot], parse_frac(&s[dot + 1..])),
        None      => (s, 0),
    }
}

fn parse_set_time(s: &str) -> Result<Ts> {
    if let Some(rest) = s.strip_prefix('@') {
        let (sec_str, nsec) = split_frac(rest);
        let sec: i64 = sec_str.parse().with_context(|| format!("invalid time '{s}'"))?;
        Ok(Ts { sec, nsec })
    } else {
        parse_datetime_str(s)
    }
}

fn parse_datetime_str(s: &str) -> Result<Ts> {
    use chrono::{Local, NaiveDateTime, TimeZone};
    let formats = [
        "%Y-%m-%d %H:%M:%S%.f",
        "%Y-%m-%dT%H:%M:%S%.f",
        "%Y-%m-%d %H:%M:%S",
        "%Y-%m-%dT%H:%M:%S",
    ];
    for fmt in &formats {
        if let Ok(ndt) = NaiveDateTime::parse_from_str(s, fmt) {
            if let Some(dt) = Local.from_local_datetime(&ndt).single() {
                return Ok(Ts {
                    sec:  dt.timestamp(),
                    nsec: dt.timestamp_subsec_nanos() as i64,
                });
            }
        }
    }
    bail!("invalid date/time '{s}'")
}

fn parse_adjust(s: &str) -> Result<(Ts, bool)> {
    let (negative, rest) = if let Some(r) = s.strip_prefix('-') {
        (true, r)
    } else {
        (false, s.strip_prefix('+').unwrap_or(s))
    };
    let (sec_str, nsec) = split_frac(rest);
    let sec: i64 = sec_str.parse().with_context(|| format!("invalid adjustment '{s}'"))?;
    if sec < 0 { bail!("invalid adjustment '{s}'"); }
    Ok((Ts { sec, nsec }, negative))
}

// ─── Display ─────────────────────────────────────────────────────────────────

fn format_ts(ts: Ts, precision: u32, use_epoch: bool) -> String {
    let divisor = 10i64.pow(9 - precision);
    let frac = ts.nsec / divisor;
    let w = precision as usize;

    if use_epoch {
        return if precision == 0 { format!("{}", ts.sec) }
               else              { format!("{}.{frac:0>w$}", ts.sec) };
    }

    use chrono::{Local, TimeZone};
    match Local.timestamp_opt(ts.sec, ts.nsec.clamp(0, 999_999_999) as u32).single() {
        Some(dt) => {
            let base = dt.format("%Y-%m-%d %H:%M:%S");
            let tz   = dt.format("%z");
            if precision == 0 { format!("{base} {tz}") }
            else               { format!("{base}.{frac:0>w$} {tz}") }
        }
        None => format!("{}.{:09}", ts.sec, ts.nsec),
    }
}

// ─── File operations ─────────────────────────────────────────────────────────

fn get_meta(path: &str, no_deref: bool) -> Result<std::fs::Metadata> {
    let p = Path::new(path);
    (if no_deref { std::fs::symlink_metadata(p) } else { std::fs::metadata(p) })
        .with_context(|| format!("cannot stat '{path}'"))
}

fn meta_times(m: &std::fs::Metadata) -> (Ts, Ts) {
    (
        Ts { sec: m.atime(), nsec: m.atime_nsec() },
        Ts { sec: m.mtime(), nsec: m.mtime_nsec() },
    )
}

fn set_file_times(path: &str, atime: Option<Ts>, mtime: Option<Ts>, no_deref: bool) -> Result<()> {
    let make_ts = |opt: Option<Ts>| -> libc::timespec {
        match opt {
            Some(ts) => libc::timespec { tv_sec: ts.sec, tv_nsec: ts.nsec },
            None     => libc::timespec { tv_sec: 0, tv_nsec: libc::UTIME_OMIT },
        }
    };
    let times = [make_ts(atime), make_ts(mtime)];
    let cpath = CString::new(path).with_context(|| format!("invalid path '{path}'"))?;
    let flags = if no_deref { libc::AT_SYMLINK_NOFOLLOW } else { 0 };
    // SAFETY: times is a valid [timespec; 2], cpath is a valid C string.
    let ret = unsafe { libc::utimensat(libc::AT_FDCWD, cpath.as_ptr(), times.as_ptr(), flags) };
    if ret != 0 {
        bail!("setting times of '{path}': {}", std::io::Error::last_os_error());
    }
    Ok(())
}

// ─── Core action ─────────────────────────────────────────────────────────────

/// Flags controlling which timestamps to show in display mode.
struct ShowFlags {
    atime: bool,
    mtime: bool,
    ctime: bool,
    btime: bool,
}

/// The resolved operation to perform on each file.
enum Action {
    Display,
    Set(Ts),
    CopyFrom(Ts, Ts), // (atime, mtime) pre-read from source
    Adjust(Ts, bool), // (delta, is_negative)
}

fn process_file(
    file: &str,
    action: &Action,
    show: &ShowFlags,
    // For write actions, only atime/mtime are writable.
    write_atime: bool,
    write_mtime: bool,
    no_dereference: bool,
    epoch: bool,
    precision: u32,
) -> Result<()> {
    match action {
        Action::Display => {
            // Use statx so we always get ctime and btime in one call.
            let ft = get_all_times(file, no_dereference)?;
            println!("{file}:");
            if show.atime {
                println!("  access: {}", format_ts(ft.atime, precision, epoch));
            }
            if show.mtime {
                println!("  modify: {}", format_ts(ft.mtime, precision, epoch));
            }
            if show.ctime {
                println!("  change: {}", format_ts(ft.ctime, precision, epoch));
            }
            if show.btime {
                match ft.btime {
                    Some(bt) => println!("   birth: {}", format_ts(bt, precision, epoch)),
                    None     => println!("   birth: -"),
                }
            }
        }
        Action::Set(ts) => {
            set_file_times(
                file,
                if write_atime { Some(*ts) } else { None },
                if write_mtime { Some(*ts) } else { None },
                no_dereference,
            )?;
        }
        Action::CopyFrom(src_at, src_mt) => {
            set_file_times(
                file,
                if write_atime { Some(*src_at) } else { None },
                if write_mtime { Some(*src_mt) } else { None },
                no_dereference,
            )?;
        }
        Action::Adjust(delta, neg) => {
            let m = get_meta(file, no_dereference)?;
            let (cur_at, cur_mt) = meta_times(&m);
            set_file_times(
                file,
                if write_atime { Some(cur_at.add(*delta, *neg)) } else { None },
                if write_mtime { Some(cur_mt.add(*delta, *neg)) } else { None },
                no_dereference,
            )?;
        }
    }
    Ok(())
}

// ─── Entry points ─────────────────────────────────────────────────────────────

fn run() -> Result<()> {
    let cli = Cli::parse();

    if !matches!(cli.precision, 0 | 3 | 6 | 9) {
        bail!("invalid precision {} (must be 0, 3, 6, or 9)", cli.precision);
    }

    // ── Display selection ────────────────────────────────────────────────────
    // If no selection flag is given, show all four timestamps.
    // If any flag is given, show only the flagged ones.
    let any_show_flag = cli.atime || cli.mtime || cli.ctime || cli.btime;
    let show = ShowFlags {
        atime: cli.atime || !any_show_flag,
        mtime: cli.mtime || !any_show_flag,
        ctime: cli.ctime || !any_show_flag,
        btime: cli.btime || !any_show_flag,
    };

    // ── Write selection (action modes) ───────────────────────────────────────
    // ctime and btime are kernel-managed and cannot be written.
    // Default: both atime and mtime; -a / -m narrow the selection.
    let write_atime = cli.atime || !cli.mtime;
    let write_mtime = cli.mtime || !cli.atime;

    // Warn if user asks to change a read-only timestamp.
    let is_action = cli.set.is_some() || cli.copy_from.is_some() || cli.adjust.is_some();
    if is_action && (cli.ctime || cli.btime) {
        eprintln!(
            "filetime: warning: change time and birth time are kernel-managed \
             and cannot be set; -c/-b ignored for write operations"
        );
    }

    // ── Resolve action ───────────────────────────────────────────────────────
    let action = if let Some(ref s) = cli.set {
        Action::Set(parse_set_time(s)?)
    } else if let Some(ref src) = cli.copy_from {
        let m = get_meta(src, cli.no_dereference)?;
        let (at, mt) = meta_times(&m);
        Action::CopyFrom(at, mt)
    } else if let Some(ref d) = cli.adjust {
        let (delta, neg) = parse_adjust(d)?;
        Action::Adjust(delta, neg)
    } else {
        Action::Display
    };

    let mut ok = true;
    for file in &cli.files {
        if let Err(e) = process_file(
            file,
            &action,
            &show,
            write_atime,
            write_mtime,
            cli.no_dereference,
            cli.epoch,
            cli.precision,
        ) {
            eprintln!("filetime: {e}");
            ok = false;
        }
    }

    if !ok { std::process::exit(1); }
    Ok(())
}

fn main() {
    if let Err(e) = run() {
        eprintln!("filetime: {e}");
        std::process::exit(1);
    }
}
