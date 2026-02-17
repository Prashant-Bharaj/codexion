/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   geometry.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/08 11:03:50 by prasingh          #+#    #+#             */
/*   Updated: 2026/02/08 11:03:58 by prasingh         ###   ########.fr       */
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

void	order_indices(int left, int right, int *first, int *second)
{
	if (left <= right)
	{
		*first = left;
		*second = right;
	}
	else
	{
		*first = right;
		*second = left;
	}
}
