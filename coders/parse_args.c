/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_args.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/05 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2026/02/08 21:56:42 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int	is_valid_int(const char *s)
{
	if (!s || !*s)
		return (0);
	if (*s == '-' || *s == '+')
		s++;
	if (!*s)
		return (0);
	while (*s)
	{
		if (*s < '0' || *s > '9')
			return (0);
		s++;
	}
	return (1);
}

static long	parse_long(const char *s)
{
	long	n;
	int		sign;

	n = 0;
	sign = 1;
	if (*s == '-')
	{
		sign = -1;
		s++;
	}
	else if (*s == '+')
		s++;
	while (*s >= '0' && *s <= '9')
	{
		n = n * 10 + (*s - '0');
		s++;
	}
	return (n * sign);
}

static int	parse_int(const char *s)
{
	int	n;
	int	sign;

	n = 0;
	sign = 1;
	if (*s == '-')
	{
		sign = -1;
		s++;
	}
	else if (*s == '+')
		s++;
	while (*s >= '0' && *s <= '9')
	{
		n = n * 10 + (*s - '0');
		s++;
	}
	return (n * sign);
}

static int	parse_scheduler(const char *s, t_params *params)
{
	if (strcmp(s, "fifo") == 0)
	{
		params->scheduler = CODEX_FIFO;
		return (0);
	}
	if (strcmp(s, "edf") == 0)
	{
		params->scheduler = CODEX_EDF;
		return (0);
	}
	return (-1);
}

int	parse_args(int argc, char **argv, t_params *params)
{
	if (argc != 9 || !params)
		return (-1);
	if (!is_valid_int(argv[1]) || !is_valid_int(argv[2])
		|| !is_valid_int(argv[3]) || !is_valid_int(argv[4])
		|| !is_valid_int(argv[5]) || !is_valid_int(argv[6])
		|| !is_valid_int(argv[7]))
		return (-1);
	params->num_coders = parse_int(argv[1]);
	params->time_to_burnout = parse_long(argv[2]);
	params->time_to_compile = parse_long(argv[3]);
	params->time_to_debug = parse_long(argv[4]);
	params->time_to_refactor = parse_long(argv[5]);
	params->num_compiles_required = parse_int(argv[6]);
	params->dongle_cooldown = parse_long(argv[7]);
	if (params->num_coders < 1)
		return (-1);
	if (params->time_to_burnout < 60 || params->time_to_compile < 60
		|| params->time_to_debug < 60 || params->time_to_refactor < 60
		|| params->num_compiles_required < 1 || params->dongle_cooldown < 60)
		return (-1);
	return (parse_scheduler(argv[8], params));
}
