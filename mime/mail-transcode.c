/* mail-encode -- convert between quoted-printable, base64 and binary
 *
 * Input is an e-mail message, output is the same message with any
 * body parts that were in quoted-printable or base64 converted to
 * binary (default), or parts in binary converted to quoted-printable
 * (option -q) or to base64 (option -b),
 *
 * E-mail messages should have line endings of CR+LF, but this program
 * also accepts single LF, to make usage easier on local files on
 * Unix. However, any parts encoded by this program will get CR+LF
 * line endings.
 *
 * Note that decoding does not yield a valid e-mail message: The
 * content-tranfer-encoding "binary" is defined by RFC 2045, but it
 * may not currently be used in messages sent over the Internet. It is
 * only meant for local processing.
 *
 * TODO: some calls to free() and regfree()
 *
 * Copyright © 2015 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/2002/copyright-software-20021231

 * Author: Bert Bos <bert@w3.org>
 * Created: 1 May 2015
 */
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <sysexits.h>
#include <string.h>
#include <regex.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define VERSION "1.0"

#define MAXMATCH 10  		/* Max # of parenthesize sub-expressions */


/* readfile -- read a whole file into a single string */
static char *readfile(int fd)
{
  char *s = NULL;
  ssize_t n, len = 0;
  char buf[8192];

  do {
    n = read(fd, buf, sizeof(buf));
    if (n == -1) err(EX_IOERR, NULL);
    s = realloc(s, (len + n + 1) * sizeof(*s));
    if (!s) err(EX_OSERR, NULL);
    strncat(s + len, buf, n);
    len += n;
  } while (n > 0);

  return s;
}


/* make_re -- compile a regular expression into rp */
static void make_re(regex_t *rp, const char *re, const char *flags)
{
  char e[1024];			/* Should be long enough for an error message */
  int r, mode = REG_EXTENDED;

  /* Flags: 'i' makes regexes case-insensitive (REG_ICASE). 'm'
   * extends '^'/'$' to also match after/before a newline and makes
   * '.' not match a newline (REG_NEWLINE). */
  for (; flags && *flags; flags++)
    if (*flags == 'i') mode |= REG_ICASE;
    else if (*flags == 'm') mode |= REG_NEWLINE;
    else errx(EX_SOFTWARE, "Unhandled flag in regex");

  r = regcomp(rp, re, mode);
  if (r) errx(EX_SOFTWARE, "%s", (regerror(r, rp, e, sizeof(e)), e));
}


/* split_one -- split a string into three parts at a regex */
static bool split_one(const char *s, const char *re, const char *flags,
		      char **before, char **match, char **after)
{
  regmatch_t pm[1];
  regex_t p;

  make_re(&p, re, flags);

  if (regexec(&p, s, 1, pm, 0) != 0) return false;

  *before = strndup(s, pm[0].rm_so);
  if (!*before) err(EX_OSERR, NULL);
  
  *match = strndup(s + pm[0].rm_so, pm[0].rm_eo - pm[0].rm_so);
  if (!*match) err(EX_OSERR, NULL);

  *after = strdup(s + pm[0].rm_eo);
  if (!*after) err(EX_OSERR, NULL);

  return true;
}


/* match -- match a regex to a string and return all sub-expressions, or false */
static bool match(const char *s, const char *re, const char *flags, char ***subs)
{
  regmatch_t pm[MAXMATCH];
  regex_t p;
  int i;

  assert(s);
  assert(re);

  make_re(&p, re, flags);

  if (regexec(&p, s, MAXMATCH, pm, 0) != 0) return false;

  if (subs) {			/* Store parenthesized sub-exprs, if requested */
    *subs = malloc(MAXMATCH * sizeof(**subs));
    if (!*subs) err(EX_OSERR, NULL);

    for (i = 0; i < MAXMATCH; i++) {
      if (pm[i].rm_so == -1)
	(*subs)[i] = strdup("");
      else
	(*subs)[i] = strndup(s + pm[i].rm_so, pm[i].rm_eo - pm[i].rm_so);
      if (!(*subs)[i]) err(EX_OSERR, NULL);
    }
  }
  return true;
}


/* split -- split a string into parts at a regex, return number of parts */
static int split(const char *s, char ***parts, const char *re, const char *flags,
		 char ***seps)
{
  regmatch_t pm[1];
  regex_t p;
  int n;
  const char *t;

  assert(s);
  assert(parts);
  assert(re);

  make_re(&p, re, flags);

  *parts = NULL;
  if (seps) *seps = NULL;
  t = s;
  n = 0;
  while (regexec(&p, t, 1, pm, 0) == 0) {
    *parts = realloc(*parts, (n + 1) * sizeof(**parts));
    if (!*parts) err(EX_OSERR, NULL);
    (*parts)[n] = strndup(t, pm[0].rm_so);
    if (!(*parts)[n]) err(EX_OSERR, NULL);

    if (seps) {
      *seps = realloc(*seps, (n + 1) * sizeof(**seps));
      if (!*seps) err(EX_OSERR, NULL);
      (*seps)[n] = strndup(t + pm[0].rm_so, pm[0].rm_eo - pm[0].rm_so);
      if (!(*seps)[n]) err(EX_OSERR, NULL);
    }

    n++;
    t += pm[0].rm_eo;
  }
  *parts = realloc(*parts, (n + 1) * sizeof(**parts));
  if (!*parts) err(EX_OSERR, NULL);
  (*parts)[n] = strdup(t);
  if (!(*parts)[n]) err(EX_OSERR, NULL);

  return n + 1;
}


/* sub -- replace matched part of a string, or return false */
static bool sub(const char *re, const char *flags, const char *repl, char **s)
{
  char *x, *y, *z;

  assert(re);
  assert(s && *s);
  assert(repl);

  if (!split_one(*s, re, flags, &x, &y, &z)) return false;

  *s = realloc(*s, (strlen(x) + strlen(repl) + strlen(z) + 1) * sizeof(**s));
  if (!*s) err(EX_OSERR, NULL);

  stpcpy(stpcpy(stpcpy(*s, x), repl), z);
  return true;
}


/* base64_val -- return the value of the character, or 64 if not valid */
static unsigned int base64_val(char c)
{
  if ('A' <= c && c <= 'Z') return c - 'A';
  if ('a' <= c && c <= 'z') return 26 + c - 'a';
  if ('0' <= c && c <= '9') return 52 + c - '0';
  if (c == '+') return 62;
  if (c == '/') return 63;
  return 64;
}


/* decode_base64 -- output the decoding of a base64 string */
static void decode_base64(const char *body)
{
  unsigned int c, v;
  int have_bits;

  for (have_bits = 0; *body && *body != '='; body++) {
    if ((v = base64_val(*body)) < 64) {
      switch (have_bits) {
      case 0: c = v; have_bits = 6; break;
      case 6: putchar((c << 2) | (v >> 4)); c = v & 0x0F; have_bits = 4; break;
      case 4: putchar((c << 4) | (v >> 2)); c = v & 0x03; have_bits = 2; break;
      case 2: putchar((c << 6) | v); c = 0; have_bits = 0; break;
      }
    }
  }
}


/* hexval -- return value of a hexadecimal digit (0-9A-F) */
static int hexval(int c)
{
  if ('0' <= c && c <= '9') return c - '0';
  assert('A' <= c && c <= 'F');
  return 10 + c - 'A';
}


/* decode_char -- return the character corresponding to "=XX" */
static int decode_char(const char *s)
{
  assert(s);
  assert(*s == '=');
  return 16 * hexval(*(s+1)) + hexval(*(s+2));
}


/* decode_quoted_printable -- decode and print a quoted-printable string */
static void decode_quoted_printable(const char *body)
{
  assert(body);
  while (*body) {
    if (*body != '=') putchar(*body++);
    else if (*(body+1) == '\r' && *(body+2) == '\n') body += 3;
    else if (*(body+1) == '\n') body += 2;
    else if (!strchr("0123456789ABCDEF", *(body+1))) putchar(*body++);
    else if (!strchr("0123456789ABCDEF", *(body+2))) putchar(*body++);
    else {putchar(decode_char(body)); body += 3;}
  }
}


/* decode -- convert a quoted-printable message to binary and print it */
static void decode(const char *message)
{
  char *head, *body, *sep, **a, **parts, **seps, *boundary;
  int n, i;

  if (!split_one(message, "\r?\n\r?\n", "", &head, &sep, &body))
    errx(EX_DATAERR, "Input must have both headers and a body");
		
  if (match(head,
    "^Content-Type:[ \t]*(\r?\n[ \t]+)*multipart/(.|\r?\n[ \t])+boundary=\"([^\"]*)\"", "im",
    &a)) {

    printf("%s%s", head, sep);

    boundary = malloc(strlen(a[3]) + 13);
    if (!boundary) err(EX_OSERR, NULL);
    stpcpy(stpcpy(stpcpy(boundary, "\r?\n--"), a[3]), "-?-?\r?\n");

    n = split(body, &parts, boundary, "", &seps);
    for (i = 1; i < n - 1; i++) {
      /* Parts 0 and (n-1) should be empty... */
      printf("%s", seps[i-1]);
      decode(parts[i]);
    }
    printf("%s", seps[n-2]);

  } else if (sub(
    "^Content-Transfer-Encoding:[ \t]*(\r?\n[ \t]+)*quoted-printable", "im",
    "Content-Transfer-Encoding: binary", &head)) {

    printf("%s%s", head, sep);
    decode_quoted_printable(body);

  } else if (sub("^Content-Transfer-Encoding:[ \t]*(\r?\n[ \t]+)*base64", "im",
		 "Content-Transfer-Encoding: binary", &head)) {

    printf("%s%s", head, sep);
    decode_base64(body);

  } else {

    printf("%s", message);

  }
}


/* encode_quoted_printable -- print a string encoded as quoted-printable */
static void encode_quoted_printable(const char *s)
{
  int n;

  assert(s);
  for (n = 0; *s; s++) {
    if (n >= 73 && *s != 10 && *s != 13) {printf("=\r\n"); n = 0;}

    if (*s == 10 || *s == 13) {putchar(*s); n = 0;}
    else if (*s<32 || *s==61 || *s>126) n += printf("=%02X", (unsigned char)*s);
    else if (*s != 32 || (*(s+1) != 10 && *(s+1) != 13)) {putchar(*s); n++;}
    else n += printf("=20");
  }
}

/* encode_base64 -- print a string encoded as base64 */
static void encode_base64(const char *s)
{
  static char c[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int i;

  for (i = 0; s[i] && s[i+1] && s[i+2]; i += 3) {
    printf("%c%c%c%c", c[s[i] >> 2], c[((s[i] & 0x03) << 4) | (s[i+1] >> 4)],
	   c[((s[i+1] & 0x0F) << 2) | (s[i+2] >> 6)], c[s[i+2] & 0x3F]);
    if (i % 57 == 54) printf("\r\n");
  }
  if (!s[i]) {
    /* No padding needed */
  } else if (!s[i+1]) {
    printf("%c%c==", c[s[i] >> 2], c[((s[i] & 0x03) << 4)]);
  } else {
    printf("%c%c%c=", c[s[i] >> 2], c[((s[i] & 0x03) << 4) | (s[i+1] >> 4)],
	   c[((s[i+1] & 0x0F) << 2)]);
  }
  printf("\r\n");
}


/* encode -- print binary message as quoted-printable or base64*/
static void encode(const char *message, const int mode)
{
  char *head, *body, *sep, **a, **parts, **seps, *boundary;
  int n, i;

  if (!split_one(message, "\r?\n\r?\n", "", &head, &sep, &body))
    errx(EX_DATAERR, "Input must have both headers and a body");
		
  if (match(head,
    "^Content-Type:[ \t]*(\r?\n[ \t]+)*multipart/(.|\r?\n[ \t])+boundary=\"([^\"]*)\"", "im",
     &a)) {

    printf("%s%s", head, sep);

    boundary = malloc(strlen(a[3]) + 13);
    if (!boundary) err(EX_OSERR, NULL);
    stpcpy(stpcpy(stpcpy(boundary, "\r?\n--"), a[3]), "-?-?\r?\n");

    n = split(body, &parts, boundary, "", &seps);
    for (i = 1; i < n - 1; i++) {
      /* Parts 0 and (n-1) should be empty... */
      printf("%s", seps[i-1]);
      encode(parts[i], mode);
    }
    printf("%s", seps[n-2]);

  } else if (mode == 1 &&
	     sub("^Content-Transfer-Encoding:[ \t]*(\r?\n[ \t]+)*binary", "im",
		 "Content-Transfer-Encoding: quoted-printable", &head)) {

    printf("%s%s", head, sep);
    encode_quoted_printable(body);

  } else if (mode == 2 &&
	     sub("^Content-Transfer-Encoding:[ \t]*(\r?\n[ \t]+)*binary", "im",
		 "Content-Transfer-Encoding: base64", &head)) {

    printf("%s%s", head, sep);
    encode_base64(body);

  } else {

    printf("%s", message);

  }
}


/* usage -- print usage message and exit */
static void usage(const char *prog)
{
  fprintf(stderr, "Usage: %s [-v] [-b] [-q] [file]\n", prog);
  exit(1);
}


/* main -- main body */
int main(int argc, char *argv[])
{
  int mode = 0;	       /* 0 = decode, 1 = encode QP, 2 = encode base64 */
  int c, in = 0;       /* Default are stdin and stdout */

  while ((c = getopt(argc, argv, ":bqv")) != -1)
    switch (c) {
    case 'b': mode = 2; break;
    case 'q': mode = 1; break;
    case 'v': printf("%s version %s\n", argv[0], VERSION); exit(0); break;
    default: usage(argv[0]);
    }
  if (argc > optind + 1) usage(argv[0]);
  if (argc > optind) in = open(argv[optind], O_RDONLY);
  if (in < 0) err(EX_NOINPUT, "%s", argv[optind]);
  if (mode) encode(readfile(in), mode); else decode(readfile(in));
  return 0;
}
