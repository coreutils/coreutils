/* spline.c -- Do spline interpolation. */

/******************************************************************************
	David L. Fox
	2140 Braun Dr.
	Golden, CO 80401

This program has been placed in the public domain by its author.

Version Date		Change
1.1	17 Dec 1985	Modify getdata() to realloc() more memory if needed.
1.0	14 May 1985

spline [options] [file]

Spline reads pairs of numbers from file (or the standard input, if file is missing).
The pairs are interpreted as abscissas and ordinates of a function.  The output of
spline consists of similar pairs generated from the input by interpolation with
cubic splines. (See R. W. Hamming, Numerical Methods for Scientists and Engineers,
2nd ed., pp. 349ff.)  There are sufficiently many points in the output to appear
smooth when plotted (e.g., by graph(1)).  The output points are approximately evenly
spaced and include the input points.

There may be one or more options of the form: -o [argument [arg2] ].

The available options are:

-a	Generate abscissa values automatically.  The input consists of a list of
	ordinates.  The generated abscissas are evenly spaced with spacing given by
	the argument, or 1 if the next argument is not a number.

-f	Set the first derivative of the spline function at the left and right end 
	points to the first and second arguments following -f.  If only one numerical
	argument follows -f then that value is used for both left and right end points.

-k	The argument of the k option is the value of the constant used to calculate
	the boundary values by y''[0] = ky''[1] and y''[n] = ky''[n-1].  The default
	value is k = 0.

-m	The argument gives the maximum number of input data points.  The default is 1000.
	If the input contains more than this many points additional memory will be
	allocated.  This option may be used to slightly increase efficiency for large
	data sets or reduce the amount of memory required for small data sets.

-n	The number of output points is given by the argument.  The default value is 100.

-p	The splines used for interpolation are forced to be periodic, i.e. y'[0] = y'[n].
	The first and last ordinates should be equal.

-s	Set the second derivative of the spline function at the left and right end 
	points to the first and second arguments following -s.  If only one numerical
	argument follows -s then that value is used for both left and right end points.

-x	The argument (and arg2, if present) are the lower (and upper) limits on the
	abscissa values in the output.  If the x option is not specified these limits
	are calculated from the data.  Automatic abscissas start at the lower limit
	(default 0).

The data need not be monotonic in x but all the x values must be distinct.
Non-numeric data in the input is ignored.
******************************************************************************/

#include	<a:stdio.h>
#include	<ctype.h>

/* The constant DOUBLE is a compile time switch.
	If #define DOUBLE appears here double pecision variables are
	used to store all data and parameters.
	Otherwise, float variables are used for the data.
	For most smoothing and and interpolation applications single
	precision is more than adequate.
	Double precision is used to solve the system of linear equations
	in either case.
*/
/* #define DOUBLE	*/

#ifdef	DOUBLE
# define	double	real; 		/* Type used for data storage. */
# define	IFMT	"%F"		/* Input format. */
# define	OFMT	"%18.14g %18.14g\n"	/* Output format. */
#else
# define	float	real;		/* Type used for data storage. */
# define	IFMT	"%f"		/* Input format. */
# define	OFMT	"%8.5g %8.5g\n"	/* Output format. */
#endif

/* Numerical constants: These may be machine and/or precision dependent. */
#define	HUGE	(1.e38)		/* Largest floating point number. */
#define	EPS	1.e-5		/* Test for zero when solving linear equations. */

/* Default parameters */
#define	NPTS	1000
#define	NINT	100
#define	DEFSTEP	1.	/* Default step size for automatic abcissas. */
#define	DEFBVK	0.	/* Default boundary value constant. */

/* Boundary condition types. */
#define	EXTRAP	0	/* Extrapolate second derivative:
			   y''(0) = k * y''(1), y''(n) = k * y''(n-1) */
#define	FDERIV	1	/* Fixed first derivatives y'(0) and y'(n). */
#define	SDERIV	2	/* Fixed second derivatives y''(0) and y''(n). */
#define	PERIOD	3	/* Periodic:  derivatives equal at end points. */

/* Token types for command line processing. */
#define	OPTION	1
#define	NUMBER	2
#define	OTHER	3

/* Define error numbers. */
#define	MEMERR	1
#define	NODATA	2
#define	BADOPT	4
#define	BADFILE	5
#define	XTRAARG	6
#define	XTRAPTS	7
#define	SINGERR	8
#define	DUPX	9
#define	BADBC	10
#define	RANGERR	11

/* Constants and flags are global. */
int aflag = FALSE;	/* Automatic abscissa flag. */
real step = DEFSTEP;	/* Spacing for automatic abscissas. */
int bdcode = EXTRAP;	/* Type of boundary conditions:
			0 extrapolate f'' with coeficient k.
			1 first derivatives given
			2 second derivatives given
			3 periodic */
real leftd = 0.,	/* Boundary values of derivatives. */
   rightd = 0.;
real k = DEFBVK;	/* Boundary value constant. */
int rflag = 0;		/* 0: take range from data, 1: min. given, 2: min. & max. given. */
real xmin = HUGE, xmax;	/* Output range. */
unsigned nint = NINT;	/* Number of intervals in output. */
unsigned maxpts = NPTS;	/* Maximun number of points. */
unsigned nknots = 0;	/* Number of input points. */
int sflag = FALSE;	/* If TRUE data must be sorted. */
char *datafile;		/* Name of data file */

int xcompare();		/* Function to compare x values of two data points. */

struct pair {
	real x, y;
	} *data;	/* Points to list of data points. */

struct bandm {
	double diag, upper, rhs;
	} *eqns;	/* Points to elements of band matrix used to calculate
			coefficients of the splines. */

main(argc, argv)
int argc;
char **argv;
{
	setup(argc, argv);
	if (nknots == 1) {	/* Cannot interpolate with one data point.  */
		printf(OFMT,data->x, data->y);
		exit(0);
	}
	if (sflag)	/* Sort data if needed. */
		qsort(data, nknots, sizeof(struct pair), xcompare);
	calcspline();
	interpolate();
}

/* xcompare -- Compare abcissas of two data points (for qsort). */
xcompare(arg1, arg2)
struct pair *arg1, *arg2;
{
	if (arg1->x > arg2->x)
		return(1);
	else if (arg1->x < arg2->x)
		return(-1);
	else
		return(0);
}

/* error -- Print error message and abort fatal errors. */
error(errno, auxmsg)
int errno;
char *auxmsg;
{	char *msg;
	int fatal, usemsg;
	static char *usage = 
	"usage: spline [options] [file]\noptions:\n",
	*options = "-a spacing\n-k const\n-n intervals\n-m points\n-p\n-x xmin xmax\n";

	fatal = TRUE;	/* Default is fatal error. */
	usemsg = FALSE;	/* Default no usage message. */
	fprintf(stderr, "spline: ");
	switch(errno) {
	case MEMERR:
		msg = "not enough memory for %u data points\n";
		break;
	case NODATA:
		msg = "data file %s empty\n";
		break;
	case BADOPT:
		msg = "unknown option: %c\n";
		usemsg = TRUE;
		break;
	case BADFILE:
		msg = "cannot open file: %s\n";
		break;
	case XTRAARG:
		msg = "extra argument ignored: %s\n";
		fatal = FALSE;
		usemsg = TRUE;
		break;
	case XTRAPTS:
		fatal = FALSE;
		msg = "%s";
		break;
	case DUPX:
		msg = "duplicate abcissa value: %s\n";
		break;
	case RANGERR:
		msg = "xmax < xmin not allowed %s\n";
		break;
	/* The following errors "can't happen." */
	/* If they occur some sort of bug is indicated. */
	case SINGERR:
		msg = "singular matrix encountered %s\n";
		break;
	case BADBC:
		msg = "internal error: bad boundary value code %d\n";
		break;
	default:
		fprintf(stderr, "unknown error number: %d\n", errno);
		exit(1);
	}
	fprintf(stderr, msg, auxmsg);
	if (usemsg)
		fprintf(stderr,"%s%s", usage, options);
	if (fatal)
		exit(1);
}

/* setup -- Initalize all constants and read data. */
setup(argc, argv)
int argc;
char **argv;
{	char *malloc();
	FILE fdinp,		/* Source of input. */
		doarg();

	fdinp = doarg(argc, argv);	/* Process command line arguments. */

	/* Allocate memory for data and band matrix of coefficients. */
	if ((data = malloc(maxpts*sizeof(struct pair))) == NULL) {
		error(MEMERR, (char *)maxpts);
	}

	getdata(fdinp);		/* Read data from fdinp. */
	if (fdinp != stdin)
		fclose(fdinp);	/* Close input data file. */
	if (nknots == 0) {
		error(NODATA, datafile);
	}
	/* Allocate memory for calculation of spline coefficients. */
	if ((eqns = malloc((nknots+1)*sizeof(struct bandm))) == NULL) {
		error(MEMERR, (char *)nknots);
	}
}

/* doarg -- Process arguments. */
FILE
doarg(argc, argv)
int argc;
char **argv;
{	int i, type;
	double atof();
	char *s, str[15];
	FILE fdinp;

	s = argv[i=1];
	type = gettok(&i, &s, argv, str);
	do {
		if (type == OPTION) {
			switch(*str) {
			case 'a':	/* Automatic abscissa. */
				aflag = TRUE;
				rflag = rflag < 2 ? 1 : rflag;
				if (xmin == HUGE)	/* Initialize xmin, if needed. */
					xmin = 0.;
				if ((type = gettok(&i, &s, argv, str)) == NUMBER) {
					if ((step = atof(str)) <= 0.)
						error(RANGERR,"");
					type = gettok(&i, &s, argv, str);
				}
				break;
			case 'f':	/* Fix first derivative at boundaries. */
				bdcode = FDERIV;
				if ((type = gettok(&i, &s, argv, str)) == NUMBER) {
					leftd = atof(str);
					if ((type = gettok(&i, &s, argv, str)) == NUMBER) {
						rightd = atof(str);
						type = gettok(&i, &s, argv, str);
					}
					else {
						rightd = leftd;
					}
				}
				break;
			case 'k':	/* Set boundary value constant. */
				bdcode = EXTRAP;
				if ((type = gettok(&i, &s, argv, str)) == NUMBER) {
					k = atof(str);
					type = gettok(&i, &s, argv, str);
				}
				break;
			case 'm':	/* Set number of intervals in output. */
				if ((type = gettok(&i, &s, argv, str)) == NUMBER) {
					maxpts = (unsigned)atoi(str);
					type = gettok(&i, &s, argv, str);
				}
				break;
			case 'n':	/* Set number of intervals in output. */
				if ((type = gettok(&i, &s, argv, str)) == NUMBER) {
					nint = (unsigned)atoi(str);
					type = gettok(&i, &s, argv, str);
				}
				break;
			case 'p':	/* Require periodic interpolation function. */
				bdcode = PERIOD;
				type = gettok(&i, &s, argv, str);
				break;
			case 's':	/* Fix second derivative at boundaries. */
				bdcode = SDERIV;
				if ((type = gettok(&i, &s, argv, str)) == NUMBER) {
					leftd = atof(str);
					if ((type = gettok(&i, &s, argv, str)) == NUMBER) {
						rightd = atof(str);
						type = gettok(&i, &s, argv, str);
					}
					else {
						rightd = leftd;
					}
				}
				break;
			case 'x':	/* Set range of x values. */
				rflag = 1;
				if ((type = gettok(&i, &s, argv, str)) == NUMBER) {
					xmin = atof(str);
					if ((type = gettok(&i, &s, argv, str)) == NUMBER) {
						xmax = atof(str);
						rflag = 2;
						type = gettok(&i, &s, argv, str);
						if (xmax <= xmin)
							error(RANGERR, "");
					}
				}
				break;
			default:
				error(BADOPT, (char *)argv[i][1]);
			}
		}
		else {
			if (argc > i) {
				datafile = argv[i];
				if ((fdinp = fopen(argv[i++], "r")) == NULL) {
					error(BADFILE, argv[i-1]);
				}
				if (argc > i)
					error(XTRAARG, argv[i]);
			}
			else
				fdinp = stdin;
			break;
		}
	} while (i < argc);
	return fdinp;
}

/* gettok -- Get one token from command line, return type. */
gettok(indexp, locp, argv, str)
int *indexp;	/* Pointer to index in argv array. */
char **locp;	/* Pointer to current location in argv[*indexp]. */
char **argv;
char *str;
{	char *s;
	char *strcpy(), *strchr();
	int type;

	s = *locp;
	while (isspace(*s) || *s == ',')
		++s;
	if (*s == '\0')		/* Look at next element in argv. */
		s = argv[++*indexp];
	if (*s == '-' && isalpha(s[1])) {
		/* Found an option. */
		*str = *++s;
		str[1] = '\0';
		++s;	
		type = OPTION;
	}
	else if (is_number(s)) {
		while (is_number(s))
			*str++ = *s++;
		*str = '\0';
		type = NUMBER;
	}
	else {
		strcpy(str, s);
		s = strchr(s, '\0');
		type = OTHER;
	}
	*locp = s;
	return(type);
}

/* is_number -- Return TRUE if argument is the ASCII representation of a number. */
is_number(string)
char *string;
{
	if (isdigit(*string) || 
	   *string == '.'    ||
	   *string == '+'   ||
	   (*string == '-' && (isdigit(string[1]) || string[1] == '.')))
		return(TRUE);
	else
		return(FALSE);
}

/* getdata -- Read data points from fdinp. */
getdata(fdinp)
FILE fdinp;
{	int n, i;
	real lastx, min, max;
	struct pair *dp;
	char msg[60], *realloc();

	nknots = 0;
	lastx = -HUGE;
	dp = data;	/* Point to head of list of data. */
	min = HUGE;
	max = -HUGE;
	do {
		if (aflag) {	/* Generate abcissa.  */
			dp->x = xmin + nknots*step;
			n = 0;
		}
		else {		/* Read abcissa. */
			while ((n = (fscanf(fdinp,IFMT,&dp->x))) == 0)
				;	/* Skip non-numeric input. */
		}
		if (n == 1) {
			if (min > dp->x)
				min = dp->x;
			if (max < dp->x)
				max = dp->x;
			if (lastx > dp->x) {		/* Check for monotonicity. */
				sflag = TRUE;	/* Data must be sorted. */
			}
			lastx = dp->x;
		}
		/* Read ordinate. */
		while ((n = (fscanf(fdinp, IFMT, &dp->y))) == 0)
			;	/* Skip non-numeric input. */
		if (++nknots >= maxpts) {		/* Too many points, allocate more memory. */
			if ((data = realloc(data, (maxpts *= 2)*sizeof(struct pair))) == NULL) {
				error(MEMERR, (char *)maxpts);
			}
			dp = data + nknots;
			sprintf(msg, "more than %d input points, more memory allocated\n",
			   maxpts/2);
			error(XTRAPTS,msg);
		}
		else
			++dp;
	} while (n == 1);
	--nknots;
	if (aflag) {	/* Compute maximum ordinate. */
		max = xmin + (nknots-1)*step;
	}
	if (rflag < 2) {
		xmax = max;
		if (rflag < 1)
			xmin = min;
	}
}

/* calcspline -- Calculate coeficients of spline functions. */
calcspline()
{
	calccoef();
	solvband();
}

/* calccoef -- Calculate coefficients of linear equations determining spline functions. */
calccoef()
{	int i, j;
	struct bandm *ptr, tmp1, tmp2;
	double h, h1;
	char str[10];

	ptr = eqns;
	/* Initialize first row of matrix. */
	if ((h1 = data[1].x - data[0].x) == 0.) {
		sprintf(str, "%8.5g", data[0].x);
		error(DUPX, str);
	}
	switch(bdcode) {	/* First equation depends on boundary conditions. */
	case EXTRAP:
		ptr->upper = -k;
		ptr->diag = 1.;
		ptr->rhs = 0.;
		break;
	case FDERIV:		/* Fixed first derivatives at ends. */
		ptr->upper = 1.;
		ptr->diag = 2.;
		h = data[1].x - data[0].x;
		ptr->rhs = 6.*((data[1].y - data[0].y)/(h*h) - leftd/h);
		break;
	case SDERIV:
		ptr->upper = 0.;
		ptr->diag = 1.;
		ptr->rhs = leftd;
		break;
	case PERIOD:		/* Periodic splines. */
		ptr->upper = 1.;
		ptr->diag = 2.;
		break;
	default:
		error(BADBC, (char *) bdcode);
	}
	++ptr;

	/* Initialize rows 1 to n-1, sub-diagonal band is assumed to be 1. */
	for (i=1; i 	< nknots-1; ++i, ++ptr) {
		h = h1;
		if ((h1 = data[i+1].x - data[i].x) == 0.) {
			sprintf(str, "%8.5g", data[i].x);
			error(DUPX, str);
		}
		ptr->diag = 2.*(h + h1)/h;
		ptr->upper = h1/h;
		ptr->rhs = 6.*((data[i+1].y-data[i].y)/(h*h1) -
		   (data[i].y - data[i-1].y)/(h*h));
	}
	/* Initialize last row. */
	switch(bdcode) {
	case EXTRAP:		/* Standard case. */
		ptr->diag = 1.;
		ptr->upper = -k;	/* Upper holds actual sub-diagonal value. */
		ptr->rhs = 0.;
		break;
	case FDERIV:		/* Fixed first derivatives at ends. */
		ptr->upper = 1.;
		ptr->diag = 2.;
		h = data[nknots-1].x - data[nknots-2].x;
		ptr->rhs = 6.*((data[nknots-2].y - data[nknots-1].y)/(h*h) + rightd/h);
		break;
	case SDERIV:
		ptr->upper = 0.;
		ptr->diag = 1.;
		ptr->rhs = rightd;
		break;
	case PERIOD:	/* Use periodic boundary conditions. */
		/* First and last row are not in tridiagonal form. */
		h = data[1].x - data[0].x;
		h1 = data[nknots-1].x - data[nknots-2].x;
		ptr->diag = 1.;
		ptr->upper = 0.;
		tmp1.diag = -1.;
		tmp1.upper = 0;
		tmp1.rhs = 0.;
		tmp2.diag = 2.*h1/h;
		tmp2.upper = h1/h;
		tmp2.rhs = 6.*((data[1].y - data[0].y)/(h*h) -
			   (data[nknots-1].y - data[nknots-2].y)/(h1*h));
		/* Transform periodic boundary equations to tri-diagonal form. */
		for (i = 1; i < nknots - 1; ++i) {
			tmp1.upper /= tmp1.diag;
			tmp1.rhs /= tmp1.diag;
			ptr->diag /= tmp1.diag;
			ptr->upper /= tmp1.diag;
			tmp1.diag = tmp1.upper - eqns[i].diag;
			tmp1.upper = -eqns[i].upper;
			tmp1.rhs -= eqns[i].rhs;
			tmp2.upper /= tmp2.diag;
			tmp2.rhs /= tmp2.diag;
			eqns->diag /= tmp2.diag;
			eqns->upper /= tmp2.diag;
			tmp2.diag = tmp2.upper - eqns[nknots-1-i].diag/eqns[nknots-1-i].upper;
			tmp2.upper = -1./eqns[nknots-1-i].upper;
			tmp2.rhs -= eqns[nknots-1-i].rhs/eqns[nknots-1-i].upper;
		}
		/* Add in remaining terms of boundary condition equation. */
		ptr->upper += tmp1.diag;
		ptr->diag += tmp1.upper;
		ptr->rhs = tmp1.rhs;
		eqns->diag += tmp2.upper;
		eqns->upper += tmp2.diag;
		eqns->rhs = tmp2.rhs;
		break;
	default:
		error(BADBC, (char *) bdcode);
	}
}

/* solvband -- Solve band matrix for spline functions. */
solvband()
{	int i, flag;
	struct bandm *ptr;
	double k1;
	double fabs();
	int fcompare();

	ptr = eqns;
	flag = FALSE;
	/* Make a pass to triangularize matrix. */
	for (i=1; i < nknots - 1; ++i, ++ptr) {
		if (fabs(ptr->diag) < EPS) {
		/* Near zero on diagonal, pivot. */
			if (fabs(ptr->upper) < EPS)
				error(SINGERR, "");
			flag = TRUE;
			ptr->diag = i;		/* Keep row index in diag.
						Actual value of diag is always 1. */
			if (i == nknots - 2) {
				flag = 2;
				/* Exchange next to last and last rows. */
				k1 = ptr->rhs/ptr->upper;
				if (fabs((ptr+1)->upper) < EPS)
					error(SINGERR, "");
				ptr->rhs = (ptr+1)->rhs/(ptr+1)->upper;
				ptr->upper = (ptr+1)->diag/(ptr+1)->upper;
				(ptr+1)->upper = 0.;
				(ptr+1)->rhs = k1;
			}
			else {
				ptr->rhs = (ptr+1)->rhs - (k1 = ptr->rhs/ptr->upper)*(ptr+1)->diag;
				ptr->upper = (ptr+1)->upper;	/* This isn't super-diagonal element
								but rather one to its right. 
								Must watch for this when 
								back substituting. */
				(++ptr)->diag = i-1;
				++i;
				ptr->upper = 0;
				ptr->rhs = k1;
				(ptr+1)->rhs -= ptr->rhs;
			}
		}
		else {
			ptr->upper /= ptr->diag;
			ptr->rhs /= ptr->diag;
			ptr->diag = i-1;		/* Used to reorder solution if needed. */
			(ptr+1)->diag -= ptr->upper;
			(ptr+1)->rhs -= ptr->rhs;
		}
	}
	/* Last row is a special case. */
	if (flag != 2) {
		/* If flag == 2 last row is already computed. */
		ptr->upper /= ptr->diag;
		ptr->rhs /= ptr->diag;
		ptr->diag = ptr - eqns;
		++ptr;
		ptr->diag -= (ptr-1)->upper * ptr->upper;
		ptr->rhs -= (ptr-1)->rhs * ptr->upper;
		ptr->rhs /= ptr->diag;
		ptr->diag = ptr - eqns;
	}

	/* Now make a pass back substituting for solution. */
	--ptr;
	for ( ; ptr >= eqns; --ptr) {
		if ((int)ptr->diag != ptr - eqns) {
			/* This row and one above have been exchanged in a pivot. */
			--ptr;
			ptr->rhs -= ptr->upper * (ptr+2)->rhs;
		}
		else
			ptr->rhs -= ptr->upper * (ptr+1)->rhs;
	}
	if (flag) {	/* Undo reordering done by pivoting. */
		qsort(eqns, nknots, sizeof(struct bandm), fcompare);
	}
}

/* fcompare -- Compare two floating point numbers. */
fcompare(arg1, arg2)
real *arg1, *arg2;
{
	if (arg1 > arg2)
		return(1);
	else if (arg1 < arg2)
		return(-1);
	else
		return(0);
}

/* interpolate -- Do spline interpolation. */
interpolate()
{	int i;
	struct pair *dp;
	struct bandm *ep;
	real h, xi, yi, hi, xu, xl, limit;
	
	h = (xmax - xmin)/nint;
	ep = eqns;
	dp = data + 1;
	for (xi = xmin; xi < xmax + 0.25*h; xi += h) {
		while (dp->x < xi && dp < data + nknots - 1) {
			++dp;	/* Skip to correct interval. */
			++ep;
		}
		if (dp < data + nknots - 1 && dp->x < xmax)
			limit = dp->x;
		else
			limit = xmax;
		for ( ; xi < limit - 0.25*h; xi += h) {
			/* Do interpolation. */
			hi = dp->x - (dp-1)->x;
			xu = dp->x - xi;
			xl = xi - (dp-1)->x;
			yi = ((ep+1)->rhs*xl*xl/(6.*hi) + dp->y/hi - (ep+1)->rhs*hi/6.)*xl +
			     (ep->rhs*xu*xu/(6.*hi) + (dp-1)->y/hi - ep->rhs*hi/6.)*xu;
			printf(OFMT, xi, yi);
		}
		if (limit != dp->x) {	/* Interpolate. */
			hi = dp->x - (dp-1)->x;
			xu = dp->x - xmax;
			xl = xmax - (dp-1)->x;
			yi = ((ep+1)->rhs*xl*xl/(6.*hi) + dp->y/hi - (ep+1)->rhs*hi/6.)*xl +
			     (ep->rhs*xu*xu/(6.*hi) + (dp-1)->y/hi - ep->rhs*hi/6.)*xu;
			printf(OFMT, xmax, yi);
		}
		else {		/* Print knot. */
			printf(OFMT, dp->x, dp->y);
			xi = dp->x;
		}
	}
}
