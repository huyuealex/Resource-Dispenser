#include <stdio.h>
#include <string.h>

struct msg {
	int prio, percent;
};

int main()
{
	char log_name[50];
	int i,j,k;
	struct msg msgs[1000];
	int cnt = 0;
	for (i = 0; i <= 0; i++)
	{
		sprintf(log_name, "log", i);
		freopen(log_name, "r", stdin);
		char str[200];
		int total = 0, flag = 0;
		cnt = 0;
		while (gets(str) > 0)
		{
			if (str[0] != '(') continue;
			int len = strlen(str);
			for (j = 0; j < len; j++)
			{
				if (str[j] == 'p' && str[j + 1] == 'r')
				{
					j = j + 7;
					break;
				}
			}
			msgs[cnt].prio = str[j] - '0';
			msgs[cnt].percent = 0;
			j = j + 4;
			for (; j < len && str[j] >= '0' && str[j] <= '9'; j++)
			{
				msgs[cnt].percent = msgs[cnt].percent * 10 + str[j] - '0';
			}
			cnt++;
		}
		for (j = 0; j < cnt; j++)
			for (k = j + 1; k < cnt; k++)
				if (msgs[j].percent == msgs[k].percent)
				{
					if (msgs[j].prio != msgs[k].prio) total++;
					if (msgs[j].prio < msgs[k].prio) flag++;
				}
		printf("%d", flag);
	}
	return 0;
}
