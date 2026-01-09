#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

void	ft_exec(char **cmd, int fd[2], int prev_fd, int has_next)
{
	if (prev_fd != -1)
	{
		if (dup2(prev_fd, STDIN_FILENO) == -1)
			exit(1);
		close(prev_fd);
	}
	if (has_next)
	{
		close(fd[0]);
		if (dup2(fd[1], STDOUT_FILENO) == -1)
			exit(1);
		close(fd[1]);
	}
	execvp(cmd[0], cmd);
	exit(1);
}

int	ft_err(int fd[2], int prev_fd, int has_next)
{
	if (has_next)
	{
		close(fd[0]);
		close(fd[1]);
	}
	if (prev_fd != -1)
		close(prev_fd);
	return (1);
}

int	ft_wait(void)
{
	int	status, exit_code = 0;

	while (wait(&status) != -1)
		if ((WIFEXITED(status) && WEXITSTATUS(status) != 0) || !WIFEXITED(status))
			exit_code = 1;
	return (exit_code);
}

int picoshell(char **cmds[])
{
	pid_t	pid;
	int		prev_fd = -1, fd[2];

	for (int i = 0; cmds[i]; i++)
	{
		if (cmds[i + 1] && pipe(fd))
			return (1);
		if ((pid = fork()) == -1)
			return (ft_err(fd, prev_fd, cmds[i + 1] != NULL));
		if (pid == 0)
			ft_exec(cmds[i], fd, prev_fd, cmds[i + 1] != NULL);
		if (prev_fd != -1)
			close(prev_fd);
		if (cmds[i + 1])
		{
			close(fd[1]);
			prev_fd = fd[0];
		}
	}
	return (ft_wait());
}
