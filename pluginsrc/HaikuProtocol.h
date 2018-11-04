#ifndef HAIKUPROTOCOL_H
#define HAIKUPROTOCOL_H

// one protocol for one protocol header file
//   you then use message->what to distinguish between individual functions
static const char* sig_haikuprotocol = "x.vnd/HaikuProtocol";

static const int32 M_GET_RANDOM_HAIKU = 'grdh';
static const int32 M_ADD_HAIKU = 'addh';

static const int32 M_RECEIVE = 'recp';

#endif
