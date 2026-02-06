/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/05 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2025/02/05 00:00:00 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

long	get_time_ms(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((long)tv.tv_sec * 1000L + (long)tv.tv_usec / 1000L);
}

long	elapsed_ms(long start)
{
	return (get_time_ms() - start);
}

void	safe_log(t_sim *sim, int coder_id, const char *msg)
{
	long	ts;

	if (!sim)
		return;
	pthread_mutex_lock(&sim->log_mutex);
	ts = get_time_ms() - sim->start_time;
	printf("%ld %d %s\n", ts, coder_id, msg);
	pthread_mutex_unlock(&sim->log_mutex);
}
