#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void str_trim_lf(char *arr, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{ // trim \n
		if (arr[i] == '\n')
		{
			arr[i] = '\0';
			break;
		}
	}
}

char *itoa(int value, char *result, int base)
{
	// check that the base if valid
	if (base < 2 || base > 36)
	{
		*result = '\0';
		return result;
	}

	char *ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do
	{
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0)
		*ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr)
	{
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

void str_overwrite_stdout()
{
	printf("\r%s", "> ");
	fflush(stdout);
}

int getOption()
{
	int ans;
	do
	{
		printf("=== WELCOME TO THE CHATROOM ===\n");
		printf("1. Log in\n");
		printf("2. Sign up\n");
		printf("3. Exist\n");
		printf("Your choice: ");
		scanf("%d", &ans);
		if (ans == 3)
		{
			printf("\nBye\n");
			return 3;
		}
	} while (ans != 1 && ans != 2);
	return ans;
}
