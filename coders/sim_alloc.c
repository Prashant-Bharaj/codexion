/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sim_alloc.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/08 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2026/02/08 00:00:00 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdlib.h>
#include <string.h>

int	init_mutexes(t_sim *sim)
{
	if (pthread_mutex_init(&sim->log_mutex, NULL) != 0)
		return (-1);
	if (pthread_mutex_init(&sim->stop_mutex, NULL) != 0)
	{
		pthread_mutex_destroy(&sim->log_mutex);
		return (-1);
	}
	return (0);
}

int	alloc_dongles(t_sim *sim)
{
	sim->dongles = (t_dongle *)malloc(sizeof(t_dongle)
			* sim->params.num_coders);
	if (!sim->dongles)
	{
		pthread_mutex_destroy(&sim->log_mutex);
		pthread_mutex_destroy(&sim->stop_mutex);
		return (-1);
	}
	memset(sim->dongles, 0, sizeof(t_dongle) * sim->params.num_coders);
	return (0);
}

int	alloc_coder_data(t_sim *sim)
{
	sim->coder_data = (t_coder_data *)malloc(sizeof(t_coder_data)
			* sim->params.num_coders);
	if (!sim->coder_data)
		return (-1);
	return (0);
}
