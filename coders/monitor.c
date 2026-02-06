/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/05 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2025/02/05 00:00:00 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <unistd.h>

void	*monitor_routine(void *arg)
{
	t_sim	*sim;
	long	now;
	long	deadline;
	int		i;

	sim = (t_sim *)arg;
	while (1)
	{
		usleep(1000);
		pthread_mutex_lock(&sim->stop_mutex);
		if (sim->stop)
		{
			pthread_mutex_unlock(&sim->stop_mutex);
			return (NULL);
		}
		pthread_mutex_unlock(&sim->stop_mutex);
		now = get_time_ms();
		i = 0;
		while (i < sim->params.num_coders)
		{
			pthread_mutex_lock(&sim->coder_data[i].mutex);
			deadline = sim->coder_data[i].last_compile_start
				+ sim->params.time_to_burnout;
			pthread_mutex_unlock(&sim->coder_data[i].mutex);
			if (now >= deadline)
			{
				pthread_mutex_lock(&sim->stop_mutex);
				sim->burnout_coder = i + 1;
				pthread_mutex_unlock(&sim->stop_mutex);
				signal_stop(sim);
				safe_log(sim, i + 1, "burned out");
				return (NULL);
			}
			i++;
		}
		pthread_mutex_lock(&sim->stop_mutex);
		if (sim->num_coders_finished >= sim->params.num_coders)
		{
			pthread_mutex_unlock(&sim->stop_mutex);
			return (NULL);
		}
		pthread_mutex_unlock(&sim->stop_mutex);
	}
	return (NULL);
}
