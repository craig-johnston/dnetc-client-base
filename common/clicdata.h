// Hey, Emacs, this a -*-C++-*- file !

// Copyright distributed.net 1997-1999 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.
//
// ****************** THIS IS WORLD-READABLE SOURCE *********************
//
//
// ----------------------------------------------------------------------
// This file contains functions for obtaining contest constants as 
// well as name, id, iteration-to-keycount-multiplication-factor or 
// obtaining/adding to contest summary data (totalblocks, totaliterations, 
// totaltime. The data itself is hidden from other modules to protect 
// integrity and ease maintenance. 
// ----------------------------------------------------------------------
//
// $Log: clicdata.h,v $
// Revision 1.17  1999/03/20 07:40:04  cyp
// Moved contestid<->proxycontestid conversion routines out. proxycontestid
// is not in public-source scope.
//
// Revision 1.16  1999/02/20 02:58:07  gregh
// Added OGR contest data, plus routines to translate between client contest
// IDs and proxy contest IDs.
//
// Revision 1.15  1999/01/29 19:05:23  jlawson
// fixed formatting.
//
// Revision 1.14  1999/01/01 02:45:14  cramer
// Part 1 of 1999 Copyright updates...
//
// Revision 1.13  1998/12/22 23:03:22  silby
// Moved rc5 cipher/iv/etc back into rsadata.h - should be in there
// because the file is shared with the proxy source.
//
// Revision 1.12  1998/12/21 18:52:53  cyp
// Added RC5 iv/cypher/plain *here*. Read the 'what this is' at the top of
// the file to see why. Also, this file has an 8.3 filename.
//
// Revision 1.11  1998/07/28 11:44:50  blast
// Amiga specific changes
//
// Revision 1.10  1998/07/15 05:49:11  ziggyb
// included the header baseincs.h because that's where timeval is and it won't compile without it being defined
//
// Revision 1.9  1998/07/07 21:55:08  cyruspatel
// client.h has been split into client.h and baseincs.
//
// Revision 1.8  1998/06/29 06:57:29  jlawson
// added new platform OS_WIN32S to make code handling easier.
//
// Revision 1.7  1998/06/22 11:25:46  cyruspatel
// Created new function in clicdata.cpp: CliClearContestSummaryData(int c)
// Needed to flush/clear accumulated statistics for a particular contest.
// Inserted into all ::SelectCore() sections that use a benchmark to select
// the fastest core. Would otherwise skew the statistics for any subsequent
// completed problem.
//
// Revision 1.6  1998/06/14 08:12:31  friedbait
// 'Log' keywords added to maintain automatic change history
//

#ifndef _CLICDATA_H_
#define _CLICDATA_H_

// return 0 if contestID is invalid, non-zero if valid.
int CliIsContestIDValid(int contestID);

// obtain the contestID for a contest identified by name.
// returns -1 if invalid name (contest not found).
int CliGetContestIDFromName( char *name );

// obtain constant data for a contest. name/iter2key may be NULL
// returns 0 if success, !0 if error (bad contestID).
int CliGetContestInfoBaseData( int contestid, const char **name, 
                                              unsigned int *iter2key );

struct timeval; /* forward ref */

// obtain summary data for a contest. unrequired args may be NULL
// returns 0 if success, !0 if error (bad contestID).
int CliGetContestInfoSummaryData( int contestid, unsigned int *totalblocks,
                                double *totaliter, struct timeval *totaltime);

// clear summary data for a contest.
// returns 0 if success, !0 if error (bad contestID).
int CliClearContestInfoSummaryData( int contestid );

// add data to the summary data for a contest.
// returns 0 if added successfully, !0 if error (bad contestID).
int CliAddContestInfoSummaryData( int contestid, unsigned int *addblocks,
                                double *aditer, struct timeval *addtime );

// Return a usable contest name, returns "???" if bad id.
const char *CliGetContestNameFromID(int contestid);

#endif //ifndef _CLICDATA_H_

