/*** size.c -- report size of various sections of an executable file */
/* Extensions/incompatibilities:
   o - BSD output has filenames at the end.
   o - BSD output can appear in different radicies.
   o - SysV output has less redundant whitespace.  Filename comes at end.
   o - SysV output doesn't show VMA which is always the same as the PMA.
   o - We also handle core files.
   o - We also handle archives.
   If you write shell scripts which manipulate this info then you may be
   out of luck; there's no +predantic switch.
*/
#include "sysdep.h"
#include "bfd.h"
#include "getopt.h"


#ifndef BSD_DEFAULT
#define BSD_DEFAULT 1
#endif

PROTO(void, display_file, (char *filename));
PROTO(void, print_sizes,  (bfd *file));

/* Various program options */

enum {decimal, octal, hex} radix = decimal;
int berkeley_format = BSD_DEFAULT; /* 0 means use AT&T-style output */
int show_version = 0;
int show_help = 0;

/* IMPORTS */
extern char *program_version;
extern char *program_name;
extern char *target;

/** main and like trivia */

void
usage ()
{
  fprintf (stderr, "size %s\nUsage: %s -{dox}{AB}V files ...\n",
	 program_version, program_name);
  fputs("\t+radix={8|10|16} -- select appropriate output radix.\n\
\t-d -- output in decimal\n\
\t-o -- output in octal\n\
\t-x -- output in hex", stderr);
  fputs("\t+format={Berkeley|SysV} -- select display format.\n\
\t-A -- SysV(AT&T) format\n\
\t-B -- BSD format", stderr);
#if BSD_DEFAULT
  fputs("\t  (Default is +format=Berkeley)", stderr);
#else
  fputs("\t  (Default is +format=SysV)", stderr);
#endif
  fputs("\t-V, +version -- display program version, etc.\n\
\t+help -- this message\n", stderr);
  exit(1);
}

struct option long_options[] = {{"radix",   1, 0, 0},
				{"format",  1, 0, 0},
				{"version", 0, &show_version, 1},
				{"target",  2, NULL, NULL},
				{"help",    0, &show_help, 1},
				{0, 0, 0, 0}};

int
main (argc, argv)
     int argc;
     char **argv;
{
  int temp;
  int c;			/* sez which option char */
  int option_index = 0;
  extern int optind;		/* steps thru options */
  program_name = *argv;

  while ((c = getopt_long(argc, argv, "ABVdox", long_options,
			  &option_index)) != EOF)
    switch(c) {
    case 0:
      if (!strcmp("format",(long_options[option_index]).name)) {
	switch(*optarg) {
	case 'B': case 'b': berkeley_format = 1; break;
	case 'S': case 's': berkeley_format = 0; break;
	default: printf("Unknown option to +format: %s\n", optarg);
	  usage();
	}
	break;
      }

      if (!strcmp("target",(long_options[option_index]).name)) {
	target = optarg;
	break;
      }

      if (!strcmp("radix",(long_options[option_index]).name)) {
#ifdef ANSI_LIBRARIES
	temp = strtol(optarg, NULL, 10);
#else
	temp = atol(optarg);
#endif
	switch(temp) {
	case 10: radix = decimal; break;
	case 8:  radix = octal; break;
	case 16: radix = hex; break;
	default: printf("Unknown radix: %s\n", optarg);
	  usage();
	}
      }
      break;
    case 'A': berkeley_format = 0; break;
    case 'B': berkeley_format = 1; break;
    case 'V': show_version = 1; break;
    case 'd': radix = decimal; break;
    case 'x': radix = hex; break;
    case 'o': radix = octal; break;
    case '?': usage();
    }

  if (show_version) printf("%s version %s\n", program_name, program_version);
  if (show_help) usage();
	
  if (berkeley_format)
#if 0	/* intel doesn't like bss/stk b/c they don't gave core files */
    puts((radix == octal) ? "text\tdata\tbss/stk\toct\thex\tfilename" :
	 "text\tdata\tbss/stk\tdec\thex\tfilename");
#else
    puts((radix == octal) ? "text\tdata\tbss\toct\thex\tfilename" :
	 "text\tdata\tbss\tdec\thex\tfilename");
#endif
  if (optind == argc)
    display_file ("a.out");
  else
    for (; optind < argc;)
      display_file (argv[optind++]);

  return 0;
}

/** Display a file's stats */

void
display_bfd (abfd)
     bfd *abfd;
{
  char *core_cmd;

  if (bfd_check_format(abfd, bfd_archive)) return;

  if (bfd_check_format(abfd, bfd_object)) {
    print_sizes(abfd);
    goto done;
  }

  if (bfd_check_format(abfd, bfd_core)) {
    print_sizes(abfd);
    fputs(" (core file", stdout);

    core_cmd = bfd_core_file_failing_command(abfd);
    if (core_cmd) printf(" invoked as %s", core_cmd);

    puts(")");
    goto done;
  }
  
  printf("Unknown file format: %s.", bfd_get_filename(abfd));

 done:


  printf("\n");
  return;
}

void
display_file(filename)
     char *filename;
{
  bfd *file, *arfile = (bfd *) NULL;

  file = bfd_openr (filename, target);
  if (file == NULL) {
    bfd_perror (filename);
    return;
  }

  if (bfd_check_format(file, bfd_archive) == true) {
    for(;;) {
      
      bfd_error = no_error;

       arfile = bfd_openr_next_archived_file (file, arfile);
      if (arfile == NULL) {
	if (bfd_error != no_more_archived_files)
	  bfd_perror (bfd_get_filename (file));
	return;
      }

      display_bfd (arfile);
      /* Don't close the archive elements; we need them for next_archive */
    }
  }
  else
    display_bfd (file);

  bfd_close (file);
}

/* This is what lexical functions are for */
void
lprint_number (width, num)
     int width, num;
{
  printf ((radix == decimal ? "%-*d\t" :
	   ((radix == octal) ? "%-*o\t" : "%-*x\t")), width, num);
}

void
rprint_number(width, num)
     int width, num;
{
  printf ((radix == decimal ? "%*d\t" :
	   ((radix == octal) ? "%*o\t" : "%*x\t")), width, num);
}

static char *bss_section_name = ".bss";
static char *data_section_name = ".data";
static char *stack_section_name = ".stack";
static char *text_section_name = ".text";

void print_berkeley_format(abfd)
bfd *abfd;
{
  sec_ptr bsssection = NULL;
  sec_ptr datasection = NULL;
  sec_ptr textsection = NULL;
  unsigned long bsssize = 0;
  unsigned long datasize = 0;
  unsigned long textsize = 0;
  unsigned long total = 0;

  
  if ((textsection = bfd_get_section_by_name (abfd, text_section_name))
      != NULL) {
    textsize = bfd_section_size (abfd, textsection);
  }

  if ((datasection = bfd_get_section_by_name (abfd, data_section_name))
      != NULL) {
    datasize = bfd_section_size(abfd, datasection);
  }
	
  if (bfd_get_format (abfd) == bfd_object) {
    if ((bsssection = bfd_get_section_by_name (abfd, bss_section_name))
	!= NULL) {
      bsssize = bfd_section_size(abfd, bsssection);
    }
  } else {
    if ((bsssection = bfd_get_section_by_name (abfd, stack_section_name))
	!= NULL) {
      bsssize = bfd_section_size(abfd, bsssection);
    }
  }
	
  total = textsize + datasize + bsssize;
	
  lprint_number (7, textsize);
  lprint_number (7, datasize);
  lprint_number (7, bsssize);
  printf (((radix == octal) ? "%-7o\t%-7x\t" : "%-7d\t%-7x\t"), total, total);

  fputs(bfd_get_filename(abfd), stdout);
  if (abfd->my_archive) printf (" (ex %s)", abfd->my_archive->filename);
}

/* I REALLY miss lexical functions! */
int svi_total = 0;

void
sysv_internal_printer(file, sec)
     bfd *file;
     sec_ptr sec;
{
  int size = bfd_section_size (file, sec);

  svi_total += size;
	
  printf ("%-12s", bfd_section_name(file, sec));
  rprint_number (8, size);
  printf(" ");
  rprint_number (8, bfd_section_vma(file, sec));
  printf ("\n");
}

void
print_sysv_format(file)
     bfd *file;
{
  svi_total = 0;

  printf ("%s  ", bfd_get_filename (file));
  if (file->my_archive) printf (" (ex %s)", file->my_archive->filename);

  puts(":\nsection\t\tsize\t     addr");
  bfd_map_over_sections (file, sysv_internal_printer, NULL);

  printf("Total       ");
  rprint_number(8, svi_total);
  printf("\n");  printf("\n");
}

void
print_sizes(file)
     bfd *file;
{
  if (berkeley_format)
    print_berkeley_format(file);
  else print_sysv_format(file);
}
