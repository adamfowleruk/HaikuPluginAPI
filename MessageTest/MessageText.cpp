#include "MessageText.h"

#include <Message.h>
#include <iostream>
#include <cstring>

int main()
{
    uint32 what = 'abcd';
    BMessage* src = new BMessage(what);
    src->AddString("poem","Lorem ipsum");
    
    std::cout << "Source message:-" << std::endl;
    src->PrintToStream();
    
    char* flat = new char[src->FlattenedSize()];
    src->Flatten(flat,src->FlattenedSize());
    std::cout << "Flat: " << flat << std::endl;
    
    BMessage* dest = new BMessage();
    dest->Unflatten(flat);
    std::cout << "Dest message:-" << std::endl;
    dest->PrintToStream();
    
    return 0;
}