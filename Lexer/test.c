int printf();

int main()
{
	int a[10];
	int i;
	int j;
	int tmp;

	a[0]=100;
	a[1]=0;
	a[2]=80;
	a[3]=101;
	a[4]=440;
	a[5]=400;
	a[6]=375;
	a[7]=102;
	a[8]=500;
	a[9]=130;

	i=0;
	j=0;

	while(j<10)
	{
		i=0;
		while(i<9)
		{
			if(a[i]<a[i+1])
			{
				tmp=a[i];
				a[i]=a[i+1];
				a[i+1]=tmp;
			}
			i=i+1;
		}
		j=j+1;
	}

	return a[0];
}

