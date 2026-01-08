#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

int picoshell(char **cmds[])
{
	int	i = 0, in_fd = 0, fd[2], pid, status;

	while (cmds[i])
	{
		if (cmds[i + 1] && pipe(fd) < 0)
			return (1);
		if ((pid = fork()) < 0)
			return (1);
		if (pid == 0)
		{
			if (in_fd != 0)
			{
				dup2(in_fd, 0);
				close(in_fd);
			}
			if (cmds[i + 1])
			{
				dup2(fd[1], 1);
				close(fd[0]);
				close(fd[1]);
			}
			execvp(cmds[i][0], cmds[i]);
			exit(1);
		}
		if (in_fd != 0)
			close(in_fd);
		if (cmds[i + 1])
		{
			close(fd[1]);
			in_fd = fd[0];
		}
		i++;
	}
	while(wait(&status))
		if (!WIFEXITED(status || WEXITSTATUS(status)))
			return (1);
	return (0);
}