#include <windows.h>
#include <iostream>
#include <cstdlib>

using namespace std;

int* numbers;
int arraySize;

CRITICAL_SECTION cs;

HANDLE startEvent;
HANDLE* blockedEvents;
HANDLE* continueEvents;
HANDLE* finishEvents;

void printArray()
{
    for (int i = 0; i < arraySize; i++)
        cout << numbers[i] << ' ';

    cout << endl;
}

DWORD WINAPI marker(LPVOID parameter)
{
    int number = *(int*)parameter;
    int markedCount = 0;

    HANDLE commands[2] = {
        finishEvents[number - 1],
        continueEvents[number - 1]
    };

    WaitForSingleObject(startEvent, INFINITE);
    srand(number);

    while (true)
    {
        int index = rand() % arraySize;

        EnterCriticalSection(&cs);

        if (numbers[index] == 0)
        {
            Sleep(5);
            numbers[index] = number;
            markedCount++;
            Sleep(5);

            LeaveCriticalSection(&cs);
        }
        else
        {
            cout << "Marker " << number
                 << ": marked elements = " << markedCount
                 << ", blocked index = " << index << endl;

            LeaveCriticalSection(&cs);

            SetEvent(blockedEvents[number - 1]);

            DWORD result = WaitForMultipleObjects(
                2, commands, FALSE, INFINITE
            );

            if (result == WAIT_OBJECT_0)
            {
                EnterCriticalSection(&cs);

                for (int i = 0; i < arraySize; i++)
                {
                    if (numbers[i] == number)
                        numbers[i] = 0;
                }

                LeaveCriticalSection(&cs);

                return 0;
            }
        }
    }
}

int main()
{
    cout << "Enter array size: ";

    if (!(cin >> arraySize) || arraySize <= 0)
    {
        cout << "Invalid array size." << endl;
        return 1;
    }

    numbers = new int[arraySize]();

    int threadCount;

    cout << "Enter number of marker threads: ";

    if (!(cin >> threadCount) || threadCount <= 0)
    {
        cout << "Invalid number of threads." << endl;
        delete[] numbers;
        return 1;
    }

    HANDLE* threads = new HANDLE[threadCount];
    int* threadNumbers = new int[threadCount];
    bool* active = new bool[threadCount];

    blockedEvents = new HANDLE[threadCount];
    continueEvents = new HANDLE[threadCount];
    finishEvents = new HANDLE[threadCount];

    InitializeCriticalSection(&cs);

    startEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (startEvent == NULL)
    {
        cout << "Cannot create the start event." << endl;
        return 1;
    }

    for (int i = 0; i < threadCount; i++)
    {
        threadNumbers[i] = i + 1;
        active[i] = true;

        blockedEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        continueEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        finishEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (blockedEvents[i] == NULL ||
            continueEvents[i] == NULL ||
            finishEvents[i] == NULL)
        {
            cout << "Cannot create thread events." << endl;
            return 1;
        }

        threads[i] = CreateThread(
            NULL, 0, marker, &threadNumbers[i], 0, NULL
        );

        if (threads[i] == NULL)
        {
            cout << "Cannot create a marker thread." << endl;
            return 1;
        }
    }

    SetEvent(startEvent);

    int remaining = threadCount;

    while (remaining > 0)
    {
        for (int i = 0; i < threadCount; i++)
        {
            if (active[i])
                WaitForSingleObject(blockedEvents[i], INFINITE);
        }

        cout << "\nArray before finishing a thread: ";
        printArray();

        int number;

        while (true)
        {
            cout << "Enter marker number to finish: ";

            if (!(cin >> number))
            {
                cout << "Invalid input." << endl;
                return 1;
            }

            if (number >= 1 && number <= threadCount &&
                active[number - 1])
                break;

            cout << "Choose an active marker thread." << endl;
        }

        SetEvent(finishEvents[number - 1]);
        WaitForSingleObject(threads[number - 1], INFINITE);

        active[number - 1] = false;
        remaining--;

        cout << "Array after finishing the thread: ";
        printArray();

        for (int i = 0; i < threadCount; i++)
        {
            if (active[i])
                SetEvent(continueEvents[i]);
        }
    }

    for (int i = 0; i < threadCount; i++)
    {
        CloseHandle(threads[i]);
        CloseHandle(blockedEvents[i]);
        CloseHandle(continueEvents[i]);
        CloseHandle(finishEvents[i]);
    }

    CloseHandle(startEvent);
    DeleteCriticalSection(&cs);

    delete[] numbers;
    delete[] threads;
    delete[] threadNumbers;
    delete[] active;
    delete[] blockedEvents;
    delete[] continueEvents;
    delete[] finishEvents;

    return 0;
}

