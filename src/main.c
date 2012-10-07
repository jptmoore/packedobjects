#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "packedobjects.h"

static int verbose_flag;

static void file_encode(packedobjectsContext *pc, const char *infile, const char *outfile, int loop);
static void file_decode(packedobjectsContext *pc, const char *infile, const char *outfile, int loop);
static void print_usage(void);
static void exit_with_message(char *message);
static const char *get_filename_ext(const char *filename);

static void file_encode(packedobjectsContext *pc, const char *infile, const char *outfile, int loop)
{
  xmlDocPtr doc = NULL;
  char *pdu = NULL;
  FILE *fp = NULL;
  int i;

  // looping all file handling
  for (i=0; i<loop; i++) {
    if ((doc = packedobjects_new_doc(infile)) == NULL) {
      exit_with_message("did not find .xml file");
    }
    pdu = packedobjects_encode(pc, doc);
    if (pc->bytes == -1) {
      fprintf(stderr, "Failed to encode with error %d.\n", pc->encode_error);
      exit(EXIT_FAILURE);
    }
    fp = fopen(outfile, "w");
    fwrite(pdu, 1, pc->bytes, fp);
    fclose(fp);
    xmlFreeDoc(doc);
  }
}

static void file_decode(packedobjectsContext *pc, const char *infile, const char *outfile, int loop)
{
  xmlDocPtr doc = NULL;
  char pdu[MAX_PDU];
  FILE *fp = NULL;
  int i;
  size_t bytes;

  // looping all file handling
  for (i=0; i<loop; i++) {
    if ((fp = fopen(infile, "r")) == NULL) {
      exit_with_message("did not find .po file");
    }
    bytes = fread(pdu, 1, MAX_PDU, fp);
    if (bytes==MAX_PDU) {
      exit_with_message(".po too large for MAX_PDU");
    }
    doc = packedobjects_decode(pc, pdu);
    if (pc->decode_error) {
      fprintf(stderr, "Failed to decode with error %d.\n", pc->decode_error);
      exit(EXIT_FAILURE);
    }
    xmlSaveFormatFileEnc(outfile, doc, "UTF-8", 1);
    fclose(fp);
    xmlFreeDoc(doc);
    xmlCleanupParser();
  }
}


static void print_usage(void)
{
  printf("usage: packedobjects --schema <file> --in <file> --out <file>\n");
  exit(EXIT_SUCCESS);
}

static void exit_with_message(char *message)
{
  printf("Failed to run: %s\n", message);
  exit(EXIT_FAILURE);
}

static const char *get_filename_ext(const char *filename)
{
  const char *dot = strrchr(filename, '.');
  if(!dot || dot == filename) return "";
  return dot + 1;
}

int main(int argc, char **argv)
{
  int c;
  packedobjectsContext *pc = NULL;
  const char *schema_file = NULL;
  const char *in_file = NULL;
  const char *out_file = NULL;
  const char *in_file_ext = NULL;
  const char *out_file_ext = NULL;
  int loop = 1;
  
  while(1) {
    static struct option long_options[] =
      {
        {"verbose", no_argument,       &verbose_flag, 1},
        {"help",  no_argument, 0, 'h'},
        {"schema",  required_argument, 0, 's'},
        {"in",  required_argument, 0, 'i'},
        {"out",  required_argument, 0, 'o'},
        {"loop",  required_argument, 0, 'l'},        
        {0, 0, 0, 0}
      };
    int option_index = 0;
    
    c = getopt_long (argc, argv, "hs:i:o:l:?", long_options, &option_index);
    
    if (c == -1) break;
    
    switch (c)
      {
      case 0:
        if (long_options[option_index].flag != 0) break;
        printf ("option %s", long_options[option_index].name);
        if (optarg) printf (" with arg %s", optarg);
        printf ("\n");
        break;

      case 'h':
        print_usage();
        break;
        
      case 's':
        schema_file = optarg;
        break;
        
      case 'i':
        in_file = optarg;
        break;
        
      case 'o':
        out_file = optarg;
        break;

      case 'l':
        loop = atoi(optarg);
        break;        
        
      case '?':
        print_usage();
        break;
        
      default:
        abort ();
      }
  }

  // do some simple checking
  if (!schema_file) exit_with_message("did not specify --schema file");
  if (!in_file) exit_with_message("did not specify --in file");
  if (!out_file) exit_with_message("did not specify --out file");

  // initialise packedobjects
  if ((pc = init_packedobjects(schema_file)) == NULL) {
    exit_with_message("failed to initialise libpackedobjects");
  }

  // check file endings to determine if encode or decode
  in_file_ext = get_filename_ext(in_file);
  out_file_ext = get_filename_ext(out_file);
  if (!(strcmp(in_file_ext, "xml")) && !(strcmp(out_file_ext, "po"))) {
    file_encode(pc, in_file, out_file, loop);
  } else if (!(strcmp(in_file_ext, "po")) && !(strcmp(out_file_ext, "xml"))) {
    file_decode(pc, in_file, out_file, loop);
  } else {
    exit_with_message("did not specify the correct file endings");
  }

  // free packedobjects
  free_packedobjects(pc);
  return EXIT_SUCCESS;
}
