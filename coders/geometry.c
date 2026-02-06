/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   geometry.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/05 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2025/02/05 00:00:00 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	get_left_dongle(int coder_id, int num_coders)
{
	if (num_coders <= 0)
		return (-1);
	return ((coder_id - 2 + num_coders) % num_coders);
}

int	get_right_dongle(int coder_id, int num_coders)
{
	if (num_coders <= 0)
		return (-1);
	return ((coder_id - 1) % num_coders);
}
