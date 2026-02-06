/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/05 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2025/02/05 00:00:00 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int	init_simulation(t_sim *sim)
{
	int	i;

	sim->start_time = get_time_ms();
	sim->stop = 0;
	sim->burnout_coder = 0;
	sim->num_coders_finished = 0;
	if (pthread_mutex_init(&sim->log_mutex, NULL) != 0)
		return (-1);
	if (pthread_mutex_init(&sim->stop_mutex, NULL) != 0)
	{
		pthread_mutex_destroy(&sim->log_mutex);
		return (-1);
	}
	sim->dongles = (t_dongle *)malloc(sizeof(t_dongle) * sim->params.num_coders);
	if (!sim->dongles)
	{
		pthread_mutex_destroy(&sim->log_mutex);
		pthread_mutex_destroy(&sim->stop_mutex);
		return (-1);
	}
	memset(sim->dongles, 0, sizeof(t_dongle) * sim->params.num_coders);
	i = 0;
	while (i < sim->params.num_coders)
	{
		sim->dongles[i].request_queue = dongle_request_queue_create(
				sim->params.scheduler);
		if (!sim->dongles[i].request_queue)
		{
			while (--i >= 0)
			{
				dongle_request_queue_destroy(sim->dongles[i].request_queue);
				dongle_destroy(&sim->dongles[i]);
			}
			free(sim->dongles);
			pthread_mutex_destroy(&sim->log_mutex);
			pthread_mutex_destroy(&sim->stop_mutex);
			return (-1);
		}
		if (dongle_init(&sim->dongles[i], sim->dongles[i].request_queue) != 0)
		{
			dongle_request_queue_destroy(sim->dongles[i].request_queue);
			while (--i >= 0)
			{
				dongle_destroy(&sim->dongles[i]);
				dongle_request_queue_destroy(sim->dongles[i].request_queue);
			}
			free(sim->dongles);
			pthread_mutex_destroy(&sim->log_mutex);
			pthread_mutex_destroy(&sim->stop_mutex);
			return (-1);
		}
		i++;
	}
	sim->coder_data = (t_coder_data *)malloc(sizeof(t_coder_data)
			* sim->params.num_coders);
	if (!sim->coder_data)
	{
		i = 0;
		while (i < sim->params.num_coders)
		{
			dongle_destroy(&sim->dongles[i]);
			dongle_request_queue_destroy(sim->dongles[i].request_queue);
			i++;
		}
		free(sim->dongles);
		pthread_mutex_destroy(&sim->log_mutex);
		pthread_mutex_destroy(&sim->stop_mutex);
		return (-1);
	}
	i = 0;
	while (i < sim->params.num_coders)
	{
		sim->coder_data[i].id = i + 1;
		sim->coder_data[i].last_compile_start = sim->start_time;
		sim->coder_data[i].compile_count = 0;
		if (pthread_mutex_init(&sim->coder_data[i].mutex, NULL) != 0)
		{
			while (--i >= 0)
				pthread_mutex_destroy(&sim->coder_data[i].mutex);
			free(sim->coder_data);
			i = 0;
			while (i < sim->params.num_coders)
			{
				dongle_destroy(&sim->dongles[i]);
				dongle_request_queue_destroy(sim->dongles[i].request_queue);
				i++;
			}
			free(sim->dongles);
			pthread_mutex_destroy(&sim->log_mutex);
			pthread_mutex_destroy(&sim->stop_mutex);
			return (-1);
		}
		i++;
	}
	return (0);
}

static void	cleanup_simulation(t_sim *sim)
{
	int	i;

	if (!sim)
		return;
	if (sim->coder_data)
	{
		i = 0;
		while (i < sim->params.num_coders)
		{
			pthread_mutex_destroy(&sim->coder_data[i].mutex);
			i++;
		}
		free(sim->coder_data);
	}
	if (sim->dongles)
	{
		i = 0;
		while (i < sim->params.num_coders)
		{
			dongle_destroy(&sim->dongles[i]);
			dongle_request_queue_destroy(sim->dongles[i].request_queue);
			i++;
		}
		free(sim->dongles);
	}
	pthread_mutex_destroy(&sim->stop_mutex);
	pthread_mutex_destroy(&sim->log_mutex);
}

int	main(int argc, char **argv)
{
	t_sim			sim;
	pthread_t		*coder_threads;
	pthread_t		monitor_thread;
	t_coder_arg		*coder_args;
	int				i;

	memset(&sim, 0, sizeof(sim));
	if (parse_args(argc, argv, &sim.params) != 0)
	{
		fprintf(stderr, "Usage: %s number_of_coders time_to_burnout "
			"time_to_compile time_to_debug time_to_refactor "
			"number_of_compiles_required dongle_cooldown scheduler\n",
			argv[0]);
		return (1);
	}
	if (init_simulation(&sim) != 0)
		return (1);
	coder_threads = (pthread_t *)malloc(sizeof(pthread_t) * sim.params.num_coders);
	coder_args = (t_coder_arg *)malloc(sizeof(t_coder_arg) * sim.params.num_coders);
	if (!coder_threads || !coder_args)
	{
		cleanup_simulation(&sim);
		free(coder_threads);
		free(coder_args);
		return (1);
	}
	i = 0;
	while (i < sim.params.num_coders)
	{
		coder_args[i].sim = &sim;
		coder_args[i].coder_id = i + 1;
		if (pthread_create(&coder_threads[i], NULL, coder_routine,
				&coder_args[i]) != 0)
		{
			signal_stop(&sim);
			while (--i >= 0)
				pthread_join(coder_threads[i], NULL);
			cleanup_simulation(&sim);
			free(coder_threads);
			free(coder_args);
			return (1);
		}
		i++;
	}
	if (pthread_create(&monitor_thread, NULL, monitor_routine, &sim) != 0)
	{
		signal_stop(&sim);
		i = 0;
		while (i < sim.params.num_coders)
		{
			pthread_join(coder_threads[i], NULL);
			i++;
		}
		cleanup_simulation(&sim);
		free(coder_threads);
		free(coder_args);
		return (1);
	}
	pthread_join(monitor_thread, NULL);
	i = 0;
	while (i < sim.params.num_coders)
	{
		pthread_join(coder_threads[i], NULL);
		i++;
	}
	cleanup_simulation(&sim);
	free(coder_threads);
	free(coder_args);
	return (sim.burnout_coder != 0);
}
