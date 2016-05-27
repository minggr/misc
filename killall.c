#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

struct pid_entry {
	pid_t pid;
	struct pid_entry *next;
	int killed;
};

/*
 * It should be getppid(void),
 * but let's assume there is a my_getppid(pid_t pid)
 */
extern pid_t my_getppid(pid_t pid);

/* Also assume below functions were implemented */
extern struct pid_entry *getAllPids(void);
struct pid_queue { /* ... */ };
extern void init_queue(struct pid_queue *);
extern void enqueue(struct pid_queue *, struct pid_entry *);
extern struct pid_entry *dequeue(struct pid_queue *);
extern int is_empty(struct pid_queue *);

int killAll(pid_t pid)
{
	struct pid_entry *pid_list, *pid_entry;
	struct pid_entry root, *parent;
	struct pid_queue q;
	int ret, err = 0;

	pid_list = getAllPids();
	if (!pid_list)
		return -ENOMEM;

	init_queue(&q);

	root.pid = pid;
	root.next = NULL;
	root.killed = 0;
	enqueue(&q, &root);

	/*
	 * Basically, the structure of all the processes is a tree
	 * So let's do a breadth first kill with a queue
	 */
	while (!is_empty(&q)) {
		parent = dequeue(&q);

		pid_entry = pid_list;
		while (pid_entry) {
			/* enqueue it if its parent is "parent" */
			if (!pid_entry->killed &&
				  my_getppid(pid_entry->pid) == parent->pid)
				enqueue(&q, pid_entry);
			pid_entry = pid_entry->next;
		}

		/*
		 * Now it's safe to kill parent because we already
		 * enqueued all it's childeren
		 */
		ret = kill(parent->pid, SIGKILL);
		if (ret) {
			printf("%d was not killed successfully,"
				"but we still continue\n", parent->pid);
			err = ret;
		}
		/* Mark it as killed no matter it's really killed or not */
		parent->killed = 1;
	}

	free(pid_list);

	return err;
}
