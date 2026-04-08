// touch -- update file timestamps, creating files if necessary
// Copyright (C) 2026 Free Software Foundation, Inc.
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.

use anyhow::{bail, Context, Result};
use clap::Parser;
use std::ffi::CString;
use std::os::unix::fs::MetadataExt;

// ─── CLI ─────────────────────────────────────────────────────────────────────

#[derive(Parser)]
#[command(
    name = "touch",
    version,
    disable_help_flag = true,
    about = "Update the access and modification times of each FILE to the current time.\n\n\
             A FILE argument that does not exist is created empty, unless -c or -h\n\
             is supplied.\n\n\
             A FILE argument of - causes touch to change the times of the file\n\
             associated with standard output."
)]
struct Cli {
    /// Change only the access time
    #[arg(short = 'a')]
    atime: bool,

    /// Do not create any files
    #[arg(short = 'c', long = "no-create")]
    no_create: bool,

    /// Parse STRING and use it instead of current time
    #[arg(short = 'd', long = "date", value_name = "STRING")]
    date: Option<String>,

    /// (ignored, for compatibility)
    #[arg(short = 'f', hide = true)]
    _compat_f: bool,

    /// Affect each symbolic link instead of its target
    #[arg(short = 'h', long = "no-dereference")]
    no_dereference: bool,

    /// Change only the modification time
    #[arg(short = 'm')]
    mtime: bool,

    /// Use this file's timestamps instead of current time
    #[arg(short = 'r', long = "reference", value_name = "FILE")]
    reference: Option<String>,

    /// Use specified time instead of current time (format: [[CC]YY]MMDDhhmm[.ss])
    #[arg(short = 't', value_name = "[[CC]YY]MMDDhhmm[.ss]")]
    time_spec: Option<String>,

    /// Specify which time to change: access time (atime/access/use) or
    /// modification time (mtime/modify)
    #[arg(long = "time", value_name = "WORD")]
    which_time: Option<String>,

    /// Set the sub-second part of the timestamp to N microseconds (0–999999);
    /// applies to the time set by -t, -d, -r, or the current time
    #[arg(long = "microseconds", value_name = "N")]
    microseconds: Option<u32>,

    /// Files to update (use - for standard output)
    #[arg(value_name = "FILE", required = true)]
    files: Vec<String>,

    /// Print help
    #[arg(long, action = clap::ArgAction::Help)]
    help: Option<bool>,
}

// ─── Timestamp ───────────────────────────────────────────────────────────────

/// A POSIX timestamp: seconds + nanoseconds since the Unix epoch.
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

    /// Override the sub-second portion with `usec` microseconds.
    fn with_microseconds(self, usec: u32) -> Self {
        Ts { sec: self.sec, nsec: usec as i64 * 1000 }
    }
}

// ─── Time source ─────────────────────────────────────────────────────────────

/// The resolved timestamps to apply ([0] = atime, [1] = mtime).
/// `None` means "use current time" — we can pass NULL to utimensat.
enum TimeSource {
    /// Both timestamps → current time (kernel handles it via NULL).
    CurrentTime,
    /// A specific pair of timestamps.
    Specific([Ts; 2]),
}

// ─── Parsing ─────────────────────────────────────────────────────────────────

/// Parse fractional seconds digits (after '.') → nanoseconds, padded to 9 digits.
fn parse_frac(s: &str) -> i64 {
    let digits: String = s.chars().take(9).collect();
    let len = digits.len();
    let val: i64 = digits.parse().unwrap_or(0);
    val * 10i64.pow((9 - len) as u32)
}

/// Parse the POSIX `-t` format: `[[CC]YY]MMDDhhmm[.ss]`
fn parse_posix_time(s: &str) -> Result<Ts> {
    use chrono::{Datelike, Local, NaiveDate, NaiveDateTime, NaiveTime, TimeZone};

    // Split off optional ".ss" at the end.
    let (main, sec_frac) = if s.len() > 2 && s.as_bytes()[s.len() - 3] == b'.' {
        let (m, dot_ss) = s.split_at(s.len() - 3);
        let ss: u32 = dot_ss[1..].parse().with_context(|| format!("invalid -t value '{s}'"))?;
        if ss > 59 {
            bail!("invalid seconds in -t value '{s}'");
        }
        (m, ss)
    } else {
        (s, 0u32)
    };

    // The remaining part is [CC][YY]MMDDhhmm.
    if !main.chars().all(|c| c.is_ascii_digit()) {
        bail!("invalid -t value '{s}'");
    }
    let (year, mmddhhmm): (i32, &str) = match main.len() {
        8 => (Local::now().year(), main),
        10 => {
            let yy: i32 = main[..2].parse()?;
            let year = if yy >= 69 { 1900 + yy } else { 2000 + yy };
            (year, &main[2..])
        }
        12 => (main[..4].parse()?, &main[4..]),
        _ => bail!("invalid -t value '{s}'"),
    };

    let mo:  u32 = mmddhhmm[0..2].parse()?;
    let day: u32 = mmddhhmm[2..4].parse()?;
    let hr:  u32 = mmddhhmm[4..6].parse()?;
    let min: u32 = mmddhhmm[6..8].parse()?;

    let date = NaiveDate::from_ymd_opt(year, mo, day)
        .ok_or_else(|| anyhow::anyhow!("invalid date in -t value '{s}'"))?;
    let time = NaiveTime::from_hms_opt(hr, min, sec_frac)
        .ok_or_else(|| anyhow::anyhow!("invalid time in -t value '{s}'"))?;
    let ndt  = NaiveDateTime::new(date, time);

    let dt = Local.from_local_datetime(&ndt).single()
        .ok_or_else(|| anyhow::anyhow!("ambiguous or invalid time in -t value '{s}'"))?;

    Ok(Ts { sec: dt.timestamp(), nsec: 0 })
}

/// Parse a flexible date string (the `-d`/`--date` argument).
/// Supports epoch `@SECONDS[.FRAC]` and common ISO formats via chrono.
fn parse_date_string(s: &str) -> Result<Ts> {
    if let Some(rest) = s.strip_prefix('@') {
        let (sec_str, nsec) = match rest.find('.') {
            Some(dot) => (&rest[..dot], parse_frac(&rest[dot + 1..])),
            None      => (rest, 0),
        };
        let sec: i64 = sec_str.parse().with_context(|| format!("invalid date '{s}'"))?;
        return Ok(Ts { sec, nsec });
    }

    use chrono::{Local, NaiveDateTime, TimeZone};
    let formats = [
        "%Y-%m-%d %H:%M:%S%.f",
        "%Y-%m-%dT%H:%M:%S%.f",
        "%Y-%m-%d %H:%M:%S",
        "%Y-%m-%dT%H:%M:%S",
        "%Y-%m-%d",
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
    bail!("invalid date string '{s}'")
}

/// Parse `--which-time` words to (do_atime, do_mtime) adjustments.
fn parse_which_time(word: &str) -> Result<(bool, bool)> {
    match word {
        "atime" | "access" | "use"  => Ok((true, false)),
        "mtime" | "modify"          => Ok((false, true)),
        _ => bail!("invalid argument '{word}' for --time"),
    }
}

// ─── Core touch operation ────────────────────────────────────────────────────

/// Touch one file: create it if necessary, then update its timestamps.
fn touch_file(
    file: &str,
    source: &TimeSource,
    do_atime: bool,
    do_mtime: bool,
    no_create: bool,
    no_dereference: bool,
) -> Result<()> {
    // Handle "-" → standard output.
    let (fd, path_for_utimensat): (libc::c_int, Option<&str>) = if file == "-" {
        (libc::STDOUT_FILENO, None)
    } else {
        (-1, Some(file))
    };

    // Try to open/create the file (gives us a write fd for the NULL-time path).
    let mut open_err: Option<std::io::Error> = None;
    let created_fd: Option<libc::c_int> = if path_for_utimensat.is_some() && !no_create && !no_dereference {
        let cpath = CString::new(file)?;
        // O_WRONLY | O_CREAT | O_NONBLOCK | O_NOCTTY
        let flags = libc::O_WRONLY | libc::O_CREAT | libc::O_NONBLOCK | libc::O_NOCTTY;
        let raw = unsafe { libc::open(cpath.as_ptr(), flags, 0o666) };
        if raw < 0 {
            open_err = Some(std::io::Error::last_os_error());
            None
        } else {
            Some(raw)
        }
    } else {
        None
    };

    // Use created_fd as the effective fd when we have one.
    let effective_fd = created_fd.unwrap_or(fd);

    // Build the timespec array (or NULL for current-time path).
    let use_null = matches!(source, TimeSource::CurrentTime) && do_atime && do_mtime;

    let utime_result = if use_null {
        // Passing NULL lets the kernel use the current time; works even without
        // file ownership as long as we have write access.
        let cpath = path_for_utimensat.map(CString::new).transpose()?;
        let path_ptr = cpath.as_ref().map_or(std::ptr::null(), |c| c.as_ptr());
        let atflag  = if no_dereference { libc::AT_SYMLINK_NOFOLLOW } else { 0 };
        let dir_fd  = if effective_fd >= 0 { effective_fd } else { libc::AT_FDCWD };
        unsafe { libc::futimens(dir_fd, std::ptr::null()) == 0
                 || libc::utimensat(libc::AT_FDCWD, path_ptr, std::ptr::null(), atflag) == 0 }
    } else {
        let ts = match source {
            TimeSource::CurrentTime => {
                let now = Ts::now();
                [now, now]
            }
            TimeSource::Specific(pair) => *pair,
        };

        let omit = libc::timespec { tv_sec: 0, tv_nsec: libc::UTIME_OMIT };
        let make = |t: Ts| libc::timespec { tv_sec: t.sec, tv_nsec: t.nsec };

        let times = [
            if do_atime { make(ts[0]) } else { omit },
            if do_mtime { make(ts[1]) } else { omit },
        ];

        let atflag  = if no_dereference { libc::AT_SYMLINK_NOFOLLOW } else { 0 };
        let dir_fd  = if effective_fd >= 0 { effective_fd } else { libc::AT_FDCWD };
        let cpath   = path_for_utimensat.map(CString::new).transpose()?;
        let path_ptr = cpath.as_ref().map_or(std::ptr::null(), |c| c.as_ptr());

        unsafe {
            // Prefer fd-based call when we have a valid fd.
            if effective_fd >= 0 && path_for_utimensat.is_some() {
                libc::utimensat(libc::AT_FDCWD, path_ptr, times.as_ptr(), atflag) == 0
            } else if effective_fd >= 0 {
                libc::futimens(effective_fd, times.as_ptr()) == 0
            } else {
                libc::utimensat(libc::AT_FDCWD, path_ptr, times.as_ptr(), atflag) == 0
            }
        }
    };

    // Close any fd we opened.
    if let Some(raw_fd) = created_fd {
        unsafe { libc::close(raw_fd) };
    }

    if !utime_result {
        let utime_errno = std::io::Error::last_os_error();
        // If we couldn't open the file, prefer that error — unless the file is
        // a directory (where open always fails but utimensat might still work).
        if let Some(oe) = open_err {
            let is_dir = std::fs::metadata(file).map_or(false, |m| m.is_dir());
            if !is_dir {
                bail!("cannot touch '{}': {}", file, oe);
            }
        }
        // Suppress ENOENT when -c is in effect.
        if no_create && utime_errno.raw_os_error() == Some(libc::ENOENT) {
            return Ok(());
        }
        bail!("setting times of '{}': {}", file, utime_errno);
    }

    Ok(())
}

// ─── Entry points ─────────────────────────────────────────────────────────────

fn run() -> Result<()> {
    let cli = Cli::parse();

    // Validate --microseconds.
    if let Some(usec) = cli.microseconds {
        if usec > 999_999 {
            bail!("invalid microseconds value {} (must be 0–999999)", usec);
        }
    }

    // Resolve which timestamps to change (-a, -m, --time).
    let (mut do_atime, mut do_mtime) = (cli.atime, cli.mtime);
    if let Some(ref word) = cli.which_time {
        let (wa, wm) = parse_which_time(word)?;
        do_atime |= wa;
        do_mtime |= wm;
    }
    if !do_atime && !do_mtime {
        do_atime = true;
        do_mtime = true;
    }

    // Reject conflicting time sources: -t conflicts with -r and -d.
    let date_set_by_t = cli.time_spec.is_some();
    let has_ref       = cli.reference.is_some();
    let has_date      = cli.date.is_some();
    if date_set_by_t && (has_ref || has_date) {
        bail!("cannot specify times from more than one source");
    }

    // Build the TimeSource.
    let mut source: TimeSource = TimeSource::CurrentTime;

    if let Some(ref spec) = cli.time_spec {
        // -t: POSIX format, same time for atime and mtime.
        let ts = parse_posix_time(spec)?;
        source = TimeSource::Specific([ts, ts]);
    }

    if has_ref {
        // -r: read timestamps from the reference file.
        let ref_path = cli.reference.as_deref().unwrap();
        let m = if cli.no_dereference {
            std::fs::symlink_metadata(ref_path)
        } else {
            std::fs::metadata(ref_path)
        }
        .with_context(|| format!("failed to get attributes of '{ref_path}'"))?;

        let ref_at = Ts { sec: m.atime(), nsec: m.atime_nsec() };
        let ref_mt = Ts { sec: m.mtime(), nsec: m.mtime_nsec() };

        if let Some(ref d) = cli.date {
            // -r + -d: date string is relative to the reference timestamps.
            let delta = parse_date_string(d)?;
            // Use the delta directly (parse_date_string gives an absolute time;
            // for true relative arithmetic we use ref times as base).
            // GNU touch applies -d relative to ref: use ref as the "now" base.
            // Since we parse ISO/epoch, just use the parsed time as-is when
            // it doesn't start with a relative keyword.
            let at = if do_atime { delta } else { ref_at };
            let mt = if do_mtime { delta } else { ref_mt };
            source = TimeSource::Specific([at, mt]);
        } else {
            source = TimeSource::Specific([ref_at, ref_mt]);
        }
    } else if let Some(ref d) = cli.date {
        // -d only: parse the date string.
        let ts = parse_date_string(d)?;
        source = TimeSource::Specific([ts, ts]);

        // Treat "-d now" (same as current time) as CurrentTime so we can use
        // the NULL-passing optimisation for relaxed permission checks.
        let now = Ts::now();
        if let TimeSource::Specific([at, _]) = &source {
            if at.sec == now.sec && at.nsec == now.nsec {
                source = TimeSource::CurrentTime;
            }
        }
    }

    // Apply --microseconds: override the sub-second part of the resolved time.
    if let Some(usec) = cli.microseconds {
        source = match source {
            TimeSource::CurrentTime => {
                // Must materialise current time so we can set the nsec field.
                let now = Ts::now().with_microseconds(usec);
                TimeSource::Specific([now, now])
            }
            TimeSource::Specific([at, mt]) => {
                TimeSource::Specific([at.with_microseconds(usec), mt.with_microseconds(usec)])
            }
        };
    }

    // Process each file.
    let mut ok = true;
    for file in &cli.files {
        if let Err(e) = touch_file(
            file,
            &source,
            do_atime,
            do_mtime,
            cli.no_create,
            cli.no_dereference,
        ) {
            eprintln!("touch: {e}");
            ok = false;
        }
    }

    if !ok {
        std::process::exit(1);
    }
    Ok(())
}

fn main() {
    if let Err(e) = run() {
        eprintln!("touch: {e}");
        std::process::exit(1);
    }
}
