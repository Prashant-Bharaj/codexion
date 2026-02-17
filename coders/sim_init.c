/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sim_init.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/08 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2026/02/08 13:47:58 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdlib.h>
#include <string.h>

static void	cleanup_on_fail(t_sim *sim, int up_to_dongle, int up_to_coder);

static int	init_dongles(t_sim *sim, int *i)
{
	*i = 0;
	while (*i < sim->params.num_coders)
	{
		sim->dongles[*i].request_queue = dongle_request_queue_create(
				sim->params.scheduler);
		if (!sim->dongles[*i].request_queue)
			return (-1);
		if (dongle_init_sim(&sim->dongles[*i],
				sim->dongles[*i].request_queue) != 0)
		{
			dongle_request_queue_destroy(sim->dongles[*i].request_queue);
			cleanup_on_fail(sim, *i, 0);
			return (-1);
		}
		(*i)++;
	}
	return (0);
}

static int	init_coder_data(t_sim *sim, int *i)
{
	*i = 0;
	while (*i < sim->params.num_coders)
	{
		sim->coder_data[*i].id = *i + 1;
		sim->coder_data[*i].last_compile_start = sim->start_time;
		sim->coder_data[*i].compile_count = 0;
		if (pthread_mutex_init(&sim->coder_data[*i].mutex, NULL) != 0)
			return (-1);
		(*i)++;
	}
	return (0);
}

static void	cleanup_on_fail(t_sim *sim, int up_to_dongle, int up_to_coder)
{
	int	j;

	j = 0;
	while (j < up_to_coder)
	{
		pthread_mutex_destroy(&sim->coder_data[j].mutex);
		j++;
	}
	if (sim->coder_data)
		free(sim->coder_data);
	j = 0;
	while (j < up_to_dongle)
	{
		dongle_destroy_sim(&sim->dongles[j]);
		dongle_request_queue_destroy(sim->dongles[j].request_queue);
		j++;
	}
	if (sim->dongles)
		free(sim->dongles);
	pthread_mutex_destroy(&sim->stop_mutex);
	pthread_mutex_destroy(&sim->log_mutex);
}

static int	init_dongles_and_data(t_sim *sim)
{
	int	i;

	if (alloc_dongles(sim) != 0)
		return (-1);
	if (init_dongles(sim, &i) != 0)
	{
		cleanup_on_fail(sim, i, 0);
		return (-1);
	}
	if (alloc_coder_data(sim) != 0)
	{
		cleanup_on_fail(sim, sim->params.num_coders, 0);
		return (-1);
	}
	if (init_coder_data(sim, &i) != 0)
	{
		cleanup_on_fail(sim, sim->params.num_coders, i);
		return (-1);
	}
	return (0);
}

int	init_simulation(t_sim *sim)
{
	sim->start_time = get_time_ms();
	sim->stop = 0;
	sim->burnout_coder = 0;
	sim->num_coders_finished = 0;
	if (init_mutexes(sim) != 0)
		return (-1);
	return (init_dongles_and_data(sim));
}
