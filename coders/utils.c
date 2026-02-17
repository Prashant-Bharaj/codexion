/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/08 11:04:29 by prasingh          #+#    #+#             */
/*   Updated: 2026/02/12 11:05:38 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

void	wake_all_dongles(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->params.num_coders)
	{
		pthread_mutex_lock(&sim->dongles[i].mutex);
		pthread_cond_broadcast(&sim->dongles[i].cond);
		pthread_mutex_unlock(&sim->dongles[i].mutex);
		i++;
	}
}

void	signal_stop(t_sim *sim)
{
	if (!sim)
		return ;
	pthread_mutex_lock(&sim->stop_mutex);
	sim->stop = 1;
	pthread_mutex_unlock(&sim->stop_mutex);
	wake_all_dongles(sim);
}

long	get_time_ms(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((long)tv.tv_sec * 1000L + (long)tv.tv_usec / 1000L);
}

int	is_stopped(t_sim *sim)
{
	int	s;

	pthread_mutex_lock(&sim->stop_mutex);
	s = sim->stop;
	pthread_mutex_unlock(&sim->stop_mutex);
	return (s);
}

void	safe_log(t_sim *sim, int coder_id, const char *msg)
{
	long	ts;

	if (!sim)
		return ;
	pthread_mutex_lock(&sim->stop_mutex);
	if (sim->stop && strcmp(msg, "burned out") != 0)
	{
		pthread_mutex_unlock(&sim->stop_mutex);
		return ;
	}
	pthread_mutex_unlock(&sim->stop_mutex);
	pthread_mutex_lock(&sim->log_mutex);
	ts = get_time_ms() - sim->start_time;
	printf("%ld %d %s\n", ts, coder_id, msg);
	pthread_mutex_unlock(&sim->log_mutex);
}
