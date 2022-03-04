/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

struct aesd_buffer_entry
{
	/**
	 * A location where the buffer contents in buffptr are stored
	 */
	const char *buffptr;
	/**
	 * Number of bytes stored in buffptr
	 */
	size_t size;
};

struct aesd_circular_buffer
{
	/**
	 * An array of pointers to memory allocated for the most recent write operations
	 */
	struct aesd_buffer_entry  entry[AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED];
	/**
	 * The current location in the entry structure where the next write should
	 * be stored.
	 */
	uint8_t in_offs;
	/**
	 * The first location in the entry structure to read from
	 */
	uint8_t out_offs;
	/**
	 * set to true when the buffer entry structure is full
	 */
	bool full;
};
/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer. 
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
			size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description 
    */
    if(buffer == NULL)
    {
    	printf("aesd_circular_buffer_add_entry(): buffer is NULL\nr");
    }
    
    if(add_entry == NULL)
    {
    	printf("aesd_circular_buffer_add_entry(): add_entry is NULL\n\r");
    }
    
    //Check if buffer is full
    if(buffer->full == 1)
    {
    	//Overwrite the oldest entry
    	memcpy(&(buffer->entry[buffer->in_offs]),add_entry);
    	buffer->in_offs += 1;
    	if(buffer->in_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
    	{
    	  buffer->in_offs = 0;
    	}
    	
    	//advance out_offs by 1
    	buffer->out_offs += 1;
    	if(buffer->out_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
    	{
    	  buffer->out_offs = 0;
    	}
    }
    else
    {
    	memcpy(&(buffer->entry[buffer->in_offs]),add_entry);
    	buffer->in_offs += 1;
    	
    	
    	//if in_offs reaches end, wrap around
    	if(buffer->in_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
    	{
    	  buffer->in_offs = 0;
    	}
    	
    	//if in_offs == out_offs
    	if(buffer->in_offs == buffer->out_offs)
    	{
    	  buffer->full = 1;
    	}
    	    	
    }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
