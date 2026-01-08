/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jjorda <jjorda@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/08 13:14:22 by jjorda            #+#    #+#             */
/*   Updated: 2026/01/08 13:20:40 by jjorda           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

int	ft_open(char type, int fd[])
{
	if (type == 'w')
	{
		close(fd[0]);
		return (fd[1]);
	}
	close(fd[1]);
	return (fd[0]);
}

int	ft_close(int fd[])
{
	close(fd[0]);
	close(fd[1]);
	return (-1);
}

int ft_popen(const char *file, char *const argv[], char type)
{
	int		fd[2];
	pid_t	pid;

	if (!file || !argv || (type != 'r' && type != 'w'))
		return (-1);
	if (pipe(fd) == -1)
		return (-1);
	if ((pid = fork()) == -1)
		return (ft_close(fd));
	if (pid == 0)
	{
		if (type == 'r')
			if (dup2(fd[0], STDIN_FILENO) == -1)
				exit(1);
		else
			if (dup2(fd[1], STDOUT_FILENO) == -1)
				exit(1);
		ft_close(fd);
		execvp(file, argv);
		exit(1);
	}
	return (ft_open(type, fd));
}
