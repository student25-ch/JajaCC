/*
 * JajaCC example
 * Copyright (C) 2026 student25-ch
 */
int main()
{
    int a = 100;
    int b = 200;
    int sum = 0;
    int i = 0;

    for (i = 0; i < 10; i = i + 1)
    {
        sum = sum + i;
    }

    if (a == 100)
    {
        a = a + b;
    }
    else
    {
        a = 0;
    }

    while (b > 0)
    {
        b = b - 50;
        if (b == 50) { break; }
    }

    return sum + a;
}
