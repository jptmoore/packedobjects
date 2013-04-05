#include <stdio.h>

#include "packedobjects.h"

#ifdef DEBUG_MODE
#define dbg(fmtstr, args...) \
  (printf(PROGNAME ":%s: " fmtstr "\n", __func__, ##args))
#else
#define dbg(dummy...)
#endif

#ifdef QUIET_MODE
#define alert(dummy...)
#else
#define alert(fmtstr, args...) \
  (fprintf(stderr, PROGNAME ":%s: " fmtstr "\n", __func__, ##args))
#endif


xmlDocPtr packedobjects_new_doc(const char *file)
{
  xmlDocPtr doc = NULL;

  xmlKeepBlanksDefault(0);
  doc = xmlReadFile(file, NULL, 0);
  
  if (doc == NULL) {
    alert("could not parse file %s", file);
  }

  return doc;
 
}

void packedobjects_dump_doc(xmlDoc *doc)
{
  xmlSaveFormatFileEnc("-", doc, "UTF-8", 1);
}

void packedobjects_dump_doc_to_file(const char *fname, xmlDoc *doc)
{
  xmlSaveFormatFileEnc(fname, doc, "UTF-8", 1);
}
