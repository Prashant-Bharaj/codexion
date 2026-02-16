/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/08 11:04:12 by prasingh          #+#    #+#             */
/*   Updated: 2026/02/08 11:04:17 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <unistd.h>

static int	check_burnout(t_sim *sim, long now)
{
	int		i;
	long	deadline;
	int		compile_count;

	i = 0;
	while (i < sim->params.num_coders)
	{
		pthread_mutex_lock(&sim->coder_data[i].mutex);
		compile_count = sim->coder_data[i].compile_count;
		deadline = sim->coder_data[i].last_compile_start
			+ sim->params.time_to_burnout;
		pthread_mutex_unlock(&sim->coder_data[i].mutex);
		if (compile_count >= sim->params.num_compiles_required)
		{
			i++;
			continue ;
		}
		if (now >= deadline)
		{
			pthread_mutex_lock(&sim->stop_mutex);
			sim->burnout_coder = i + 1;
			pthread_mutex_unlock(&sim->stop_mutex);
			signal_stop(sim);
			safe_log(sim, i + 1, "burned out");
			return (i + 1);
		}
		i++;
	}
	return (0);
}

static int	should_exit(t_sim *sim, long now)
{
	if (check_burnout(sim, now))
		return (1);
	pthread_mutex_lock(&sim->stop_mutex);
	if (sim->num_coders_finished >= sim->params.num_coders)
	{
		pthread_mutex_unlock(&sim->stop_mutex);
		return (1);
	}
	pthread_mutex_unlock(&sim->stop_mutex);
	return (0);
}

void	*monitor_routine(void *arg)
{
	t_sim	*sim;
	long	now;

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
		if (should_exit(sim, now))
			return (NULL);
	}
	return (NULL);
}
