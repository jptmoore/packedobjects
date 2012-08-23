
#include "packedobjects.h"

int main()
{
  packedobjectsContext *pc = NULL;
  xmlDocPtr doc1 = NULL;
  xmlDocPtr doc2 = NULL;
  char *pdu = NULL;

  pc = init_packedobjects((const char *) "../examples/personnel.xsd");
  if (pc) {
    doc1 = packedobjects_new_doc((const char *) "../examples/personnel.xml");
    pdu = packedobjects_encode(pc, doc1);
    printf("bytes:%d\n", pc->bytes);
    doc2 = packedobjects_decode(pc, pdu);
    packedobjects_dump_doc(doc2);
    xmlFreeDoc(doc2);
    xmlFreeDoc(doc1);
    free_packedobjects(pc);
    return 0;
  } else {
    printf("Bork!\n");
    return -1;
  }
}
