/*****************************************************************************\
 *  job_info.c - get/print the job step state information of slurm
 *****************************************************************************
 *  Copyright (C) 2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Moe Jette <jette1@llnl.gov>, Joey Ekstrom <ekstrom1@llnl.gov> et. al.
 *  UCRL-CODE-2002-040.
 *  
 *  This file is part of SLURM, a resource management program.
 *  For details, see <http://www.llnl.gov/linux/slurm/>.
 *  
 *  SLURM is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *  
 *  SLURM is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with ConMan; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <src/api/slurm.h>
#include <src/common/slurm_protocol_api.h>

/* slurm_print_job_step_info_msg - output information about all Slurm job steps */
void 
slurm_print_job_step_info_msg ( FILE* out, job_step_info_response_msg_t * job_step_info_msg_ptr )
{
	int i;
	job_step_info_t * job_step_ptr = job_step_info_msg_ptr -> job_steps ;

	fprintf( out, "Job steps updated at %ld, record count %d\n",
		(long) job_step_info_msg_ptr ->last_update, job_step_info_msg_ptr->job_step_count);

	for (i = 0; i < job_step_info_msg_ptr-> job_step_count; i++) 
	{
		slurm_print_job_step_info ( out, & job_step_ptr[i] ) ;
	}
}

/* slurm_print_job_step_info - output information about a specific Slurm job step */
void
slurm_print_job_step_info ( FILE* out, job_step_info_t * job_step_ptr )
{
	int j;

	fprintf ( out, "JobId=%u StepId=%u UserId=%u ", 
		job_step_ptr->job_id, job_step_ptr->step_id, job_step_ptr->user_id);
	fprintf ( out, "StartTime=%ld Partition=%s Nodes=%s\n\n", 
		(long)job_step_ptr->start_time, job_step_ptr->partition, job_step_ptr->nodes);
}

/* slurm_load_job_steps - issue RPC to get Slurm job_step state information */
int
slurm_get_job_steps ( uint32_t job_id, uint32_t step_id, job_step_info_response_msg_t **step_response_pptr)
{
	int msg_size ;
	int rc ;
	slurm_fd sockfd ;
	slurm_msg_t request_msg ;
	slurm_msg_t response_msg ;

	job_step_info_request_msg_t step_request;
	return_code_msg_t * slurm_rc_msg ;

	/* init message connection for message communication with controller */
	if ( ( sockfd = slurm_open_controller_conn ( ) ) == SLURM_SOCKET_ERROR ) {
		slurm_seterrno ( SLURM_COMMUNICATIONS_CONNECTION_ERROR );
		return SLURM_SOCKET_ERROR ;
	}

	/* send request message */
	step_request . job_id = job_id ;
	step_request . job_step_id = step_id ;
	request_msg . msg_type = REQUEST_JOB_STEP_INFO ;
	request_msg . data = &step_request;
	if ( ( rc = slurm_send_controller_msg ( sockfd , & request_msg ) ) == SLURM_SOCKET_ERROR ) {
		slurm_seterrno ( SLURM_COMMUNICATIONS_SEND_ERROR );
		return SLURM_SOCKET_ERROR ;
	}

	/* receive message */
	if ( ( msg_size = slurm_receive_msg ( sockfd , & response_msg ) ) == SLURM_SOCKET_ERROR ) {
		slurm_seterrno ( SLURM_COMMUNICATIONS_RECEIVE_ERROR );
		return SLURM_SOCKET_ERROR ;
	}

	/* shutdown message connection */
	if ( ( rc = slurm_shutdown_msg_conn ( sockfd ) ) == SLURM_SOCKET_ERROR ) {
		slurm_seterrno ( SLURM_COMMUNICATIONS_SHUTDOWN_ERROR );
		return SLURM_SOCKET_ERROR ;
	}
	if ( msg_size )
		return msg_size;

	switch ( response_msg . msg_type )
	{
		case RESPONSE_JOB_STEP_INFO:
			*step_response_pptr = ( job_step_info_response_msg_t * ) response_msg . data ;
			return SLURM_PROTOCOL_SUCCESS ;
			break ;
		case RESPONSE_SLURM_RC:
			slurm_rc_msg = ( return_code_msg_t * ) response_msg . data ;
			rc = slurm_rc_msg->return_code;
			slurm_free_return_code_msg ( slurm_rc_msg );	
			if (rc) {
				slurm_seterrno ( rc );
				return SLURM_PROTOCOL_ERROR;
			}
			break ;
		default:
			slurm_seterrno ( SLURM_UNEXPECTED_MSG_ERROR );
			return SLURM_PROTOCOL_ERROR;
			break ;
	}

	return SLURM_PROTOCOL_SUCCESS ;
}

