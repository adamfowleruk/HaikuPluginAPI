#ifndef HAIKUPROTOCOL_H
#define HAIKUPROTOCOL_H

// one protocol for one protocol header file
//   you then use message->what to distinguish between individual functions
const char* sig_haikuprotocol = "x.vnd/HaikuProtocol";

const int32 M_GET_RANDOM_HAIKU = 'grdh';
const int32 M_ADD_HAIKU = 'addh';

const int32 M_RECEIVE = 'recp';

#endif
