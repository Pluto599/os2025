#include <stdio.h>
#include <cstdio>

int main()
{
	printf("Now I will test your getChar: ");
	printf("1 + 1 = ");
	char num = getchar();
	printf("%c * 123 = 246\n", num);
	printf("Now I will test your getStr: ");
	printf("Alice is stronger than ");
	char name[20];
	// gets(name);
	scanf("%19s", name);
	printf("%s is weaker than Alice\n", name);

	return 0;
}