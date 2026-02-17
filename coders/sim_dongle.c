/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sim_dongle.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/08 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2026/02/08 00:00:00 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdlib.h>

int	dongle_init_sim(t_dongle *d, void *queue)
{
	if (!d || !queue)
		return (-1);
	if (pthread_mutex_init(&d->mutex, NULL) != 0)
		return (-1);
	if (pthread_cond_init(&d->cond, NULL) != 0)
	{
		pthread_mutex_destroy(&d->mutex);
		return (-1);
	}
	d->cooldown_until = 0;
	d->holder = -1;
	d->request_queue = queue;
	return (0);
}

void	dongle_destroy_sim(t_dongle *d)
{
	if (!d)
		return ;
	pthread_cond_destroy(&d->cond);
	pthread_mutex_destroy(&d->mutex);
}

void	dongle_release(t_dongle *d, t_sim *sim)
{
	long	now;

	if (!d || !sim)
		return ;
	pthread_mutex_lock(&d->mutex);
	d->holder = -1;
	now = get_time_ms();
	d->cooldown_until = now + sim->params.dongle_cooldown;
	pthread_mutex_unlock(&d->mutex);
	wake_all_dongles(sim);
}
