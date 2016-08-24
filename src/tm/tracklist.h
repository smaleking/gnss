/*
 * tracklist.h
 *
 *  Created on: Aug 10, 2016
 *      Author: dma
 */

#ifndef TRACKLIST_H_
#define TRACKLIST_H_

#include "tm.h"

#define	TRACK_LIST_MAX (32)

// all channel struct
typedef struct track_array_list_s{
	track_information info[TRACK_LIST_MAX];
	int capacity;
} track_array_list;

// 1. initialzie teh track_arrayList object
void init_track_array_list(track_array_list* track_info_array_list);

// 2. check if a satellite is in track list
int check_satellite_in_track_array_list(track_array_list *track_info_array_list, int prn);

// 3. add a channel to channel list
void add_track_info_to_array_list(track_array_list *track_info_array_list, track_information *one_satellite_info);

// 4. remove a channel from channel list
void remove_track_info_from_array_list(track_array_list * track_info_array_list, int prn);

// 5. get the active channel size
int track_info_array_list_size(track_array_list *track_info_array_list);

// 6. get track info
track_information *get_track_info(track_array_list *track_info_array_list, int index);

#endif /* TRACKLIST_H_ */
