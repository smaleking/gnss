/*
 * tracklist.c
 *
 *  Created on: Aug 10, 2016
 *      Author: Kunlun Yan
 */
#include "tracklist.h"

// 1. initialize a track_array_list (channels)
void init_track_array_list(track_array_list* track_info_array_list)
{
	track_info_array_list->capacity = 0;
}

// 2. check if a satellite is in the track_array_list
int check_satellite_in_track_array_list(track_array_list *track_info_array_list, int prn)
{
	int i;
	for(i = 0;i<track_info_array_list->capacity;i++)
	{
		if(track_info_array_list->info[i].prn == prn)
		{
			return 1;
		}
	}
	return 0;
}

// 3. add a channel to track_array_list
void add_track_info_to_array_list(track_array_list *track_info_array_list, track_information *one_satellite_info)
{
	track_info_array_list->info[track_info_array_list->capacity] = *one_satellite_info;
	track_info_array_list->capacity++;
}

// 4. remove a prn from track_array_list
void remove_track_info_from_array_list(track_array_list * track_info_array_list, int prn)
{
	int i;
	for(i = 0;i<track_info_array_list->capacity;i++)
	{
		if(track_info_array_list->info[i].prn == prn)
		{
			break;
		}
	}
	for(;i<track_info_array_list->capacity-1;i++)
	{
		track_info_array_list->info[i] = track_info_array_list->info[i+1];
	}
	track_info_array_list->capacity--;
}

// 5. get track_array_list size
int track_info_array_list_size(track_array_list *track_info_array_list)
{
	return track_info_array_list->capacity;
}

// 6. get the channel info given a channel index
track_information *get_track_info(track_array_list *track_info_array_list, int index)
{
	return &(track_info_array_list->info[index]);
}


