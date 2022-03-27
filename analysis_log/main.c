#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

/**
 * Author: Loi Dinh
 * Date: 2022-03-01
 *
 * Read and analysis log
 */

#define FILENAME "log.txt"
#define MAX_LEN_FILE 5000

char fileStr[MAX_LEN_FILE];

int fileToStr(char *str);

int countNumberOfMessages();
void countMessagesForDevice(const char *device);
void getSwitches();
int countNumberOfErrorMessages();
int calculateMaxDelayBetweenSendAndReceive();
int calculateAverageDelay();

void processMessageCode(const int *i, char arr[][100]);
char *multi_tok(char *input, char *delimiter);
void splitTimestamp(char *date_token, char *hour_token, const char time[], char timestamp[][7]);
void calculateDiff(int *totalDiff, const char timestamp1[][7], const char timestamp2[][7]);
void parseMessageForTimeAndCode(int *i, char time[][30], char code[][100]);


int main() {

    fileToStr(fileStr);
    printf("Content of file %s: \n%s", FILENAME, fileStr);

    printf("\nNumber of messages: %d\n", countNumberOfMessages());

    char device[10];
    printf("\nInput network device: ");
    scanf("%s", device);
    countMessagesForDevice(device);

    getSwitches();

    printf("\nNumber of error message: %d\n", countNumberOfErrorMessages());

    printf("\nMax delay: %d millisecond\n", calculateMaxDelayBetweenSendAndReceive());

    printf("\nAverage delay: %d millisecond", calculateAverageDelay());

    return 0;
}

int fileToStr(char *str)
{
    int status;
    FILE *fp = NULL;

    fp = fopen("../" FILENAME, "r");
    if (fp == NULL)
    {
        printf("File does not exists\n");
        return -1;
    }
    status = fread(str, MAX_LEN_FILE, 1, fp);

    fclose(fp);
    fp = NULL;
    return status;
}

/**
 * Count messages was sent in the log time
 * Use strstr to find substring
 * and move forward to the end of file
 * @return
 */
int countNumberOfMessages()
{

    char cmdSet[] = "\"cmd\":\"set\"";
    char *pCount = fileStr;
    int count = 0;

    while (*pCount != '\0'
           && ((pCount = strstr(pCount, cmdSet)) != NULL))
    {
        // Move a character forward to count the next cmdSet
        ++pCount;

        // Increase count
        ++count;
    }

    return count;
}

/**
 * Messages for a specific device
 * Find the string that has the pattern like input device and cmdSet by using strtok to get field
 * and strstr to find pattern
 * Use 2 copies of the message to reuse
 *
 * @param device
 */
void countMessagesForDevice(const char *device)
{
    int countMessagesOfDevice = 0;
    char token_msg_1[200];              // first copy to traverse
    char token_msg_2[200];              // second copy to print
    char *end_str;
    char cmdSet[] = "\"cmd\":\"set\"";

    // Extract each message with enter "\n"
    char *token_msg = strtok_r(fileStr, "\n", &end_str);
    while (token_msg != NULL)
    {
        if (strstr(token_msg, cmdSet)
            && strstr(token_msg, device))
        {
            strcpy(token_msg_1, token_msg);
            strcpy(token_msg_2, token_msg);

            // Split data in array "[{" with the first and second part
            char *token_field = multi_tok(token_msg_1, "[{");
            token_field = multi_tok(NULL, "[{");

            // Split data in second part from above array by ","
            char *token_data = strtok(token_field, ",");
            while (token_data != NULL)
            {
                printf("data: %s\n", token_data);
                // Check key is "data" and value is input device
                if (strstr(token_data, "data")
                    && strstr(token_data, device))
                {
                    // Use the second copy to print
                    printf("%s\n", token_msg_2);
                    ++countMessagesOfDevice;
                    break;
                }
                token_data = strtok(NULL, ",");
            }
        }
        token_msg = strtok_r(NULL, "\n", &end_str);
    }
    printf("\nNumber of messages was sent for this device: %d\n", countMessagesOfDevice);
}

/**
 * Get information on switches that have interaction with the central controller
 * Find key-value include cmd-set and type-switch in each message
 * Then split value of data key of the message into 3 elements, and use 2nd and 3rd element
 * Also check duplicate value for each network - endpoint pair
 *
 */
void getSwitches()
{
    fileToStr(fileStr);

    int j, number = 0,  same = 0;
    char networkArr[10][100], endpointArr[10][10];
    char cmdSet[] = "\"cmd\":\"set\"";
    char switchType[] = "\"type\":\"switch\"";

    char data1[120], *end_str;

    // Extract messages
    char* token_msg = strtok_r(fileStr, "\n", &end_str);
    while (token_msg != NULL)
    {
        // Data was sent from central controller with set cmd to switch
        if (strstr(token_msg, cmdSet)
            && strstr(token_msg, switchType))
        {
            // Copy from data object in json (begin with "data")
            strcpy(data1, strstr(token_msg, "data"));

            // Only get 1st element in the string that was split by ","
            char* processData = strtok(data1, ",");

            char data_el[3][10];    // split data into 3 elements by "-", example: data":["zwave-dc53:4-1"]
            processData  = strtok(processData, "-");
            int k = 0;
            while (processData != NULL)
            {
                strcpy(data_el[k], processData);
                ++k;
                processData = strtok(NULL, "-");
            }

            // Split 2nd element by ":",
            // then copy and only using the first one
            strcpy(data_el[1], strtok(data_el[1], ":"));

            // The 3rd element will contain some characters that will not use
            // Remove last characters
            for (int m = 0; data_el[2][m] != '\0'; m++)
            {
                if (data_el[2][m] == ']'
                    || data_el[2][m] == '\"') {
                    data_el[2][m] = '\0';
                }
            }

            // Check if data is duplicate
            for (int n = 0; n < 10; n++)
            {
                if (strcmp(networkArr[n], data_el[1]) == 0
                    && strcmp(endpointArr[n], data_el[2]) == 0)
                {
                    same = 1;
                }
            }

            // If data is not duplicate, then use it
            if (same == 0)
            {
                strcpy(networkArr[number], data_el[1]);
                strcpy(endpointArr[number], data_el[2]);
                ++number;
            }
        }
        token_msg = strtok_r(NULL, "\n", &end_str);
    }

    for (j = 0; j < number; j++)
    {
        printf("\nDevice %i has address: NWK - %s, ENDPOINT - %s\n", j + 1, networkArr[j], endpointArr[j]);
    }

}

/**
 * Number of error message
 * Split each message by using strstr to get only "reqid" part
 * Then get message pairs in adjacent order i and i+1 to compare value of key "reqid"
 * If value of key "reqid" is different, the message has an error
 *
 * @return
 */
int countNumberOfErrorMessages()
{
    fileToStr(fileStr);

    int validMsg = 0, numberError = 0, i, j;
    char msg[19][100];

    // Extract messages
    char *token_msg = strtok(fileStr, "\n");
    while (token_msg != NULL)
    {
        strcpy(msg[validMsg], strstr(token_msg, "reqid"));

        ++validMsg;
        token_msg = strtok(NULL, "\n");
    }

    char sent[10], receive[10];
    for (i = 0; i < validMsg; i++)
    {
        // Sent package
        char *sent_token = strtok((char *) *(msg + i), ":");
        strcpy(sent, strtok(NULL, ":"));

        // Received package
        char *receive_token = strtok((char *) *(msg + ++i), ":");
        strcpy(receive, strtok(NULL, ":"));

        // Remove last character
        for (j = 0; sent[j] != '\0'; j++)
        {
            if (sent[j] == '}') {
                sent[j] = '\0';
            }
        }

        for (j = 0; receive[j] != '\0'; j++)
        {
            if (receive[j] == '}') {
                receive[j] = '\0';
            }
        }

        if (strcmp(sent, receive) != 0) {
            numberError++;
        }
    }
    return numberError;
}

/**
 * Get code value and remove redundant characters
 * @param i
 * @param arr
 */
void processMessageCode(const int *i, char arr[][100])
{
    char code[10];
    for (int j = 0; j < *i; j++)
    {
        char *code_token = strtok((char *) *(arr + j), ":");
        strncpy(code, strtok(NULL, ":"), 10);

        // Remove last character
        int m;
        for (m = 0; code[m] != '\0'; m++)
        {
            if (code[m] == '}') {
                code[m] = '\0';
            }
        }
        strcpy(arr[j], code);

    }
}

/**
 * Max delay time
 * Find the messages that have the same "reqid" of sent and received messages
 * Then find the max delay from each total difference in the millisecond
 *
 * @return
 */
int calculateMaxDelayBetweenSendAndReceive()
{
    fileToStr(fileStr);

    int maxDelay = 0;
    int i = 0, j;
    char code[19][100], time[19][30];
    int totalDiff;

    parseMessageForTimeAndCode(&i, time, code);

    processMessageCode(&i, code);

    for (j = 0; j < i; j += 2)
    {
        // Compare successful sent and receive messages with the same code
        if (strcmp(*(code + j), *(code + j + 1)) == 0)
        {
            char timestamp1[7][7], timestamp2[7][7];  // 7 fields of time, max length 7 (second and millisecond - ex: 45.638)
            char date_token[11], hour_token[12];

            // Process date 1
            splitTimestamp(date_token, hour_token, *(time + j), timestamp1);

            // Process date 2
            splitTimestamp(date_token, hour_token, *(time + j + 1), timestamp2);

            calculateDiff(&totalDiff, timestamp1, timestamp2);

            if (totalDiff > maxDelay)
                maxDelay = totalDiff;
        }
    }
    return maxDelay;
}

/**
 * Calculate total time of each timestamp into millisecond,
 * then calculate the difference of two
 * @param totalDiff
 * @param timestamp1
 * @param timestamp2
 */
void calculateDiff(int *totalDiff, const char timestamp1[][7], const char timestamp2[][7])
{
    int total1, total2;
    total2 = (atoi(timestamp2[0]) * 365 * 24 * 60 * 60 * 1000)
             + (atoi(timestamp2[1]) * 30 * 24 * 60 * 60 * 1000)
             + (atoi(timestamp2[2]) * 24 * 60 * 60 * 1000)
             + (atoi(timestamp2[3]) * 60 * 60 * 1000)
             + (atoi(timestamp2[4]) * 60 * 1000)
             + (atoi(timestamp2[5]) * 1000)
             + atoi(timestamp2[6]);

    total1 = (atoi(timestamp1[0]) * 365 * 24 * 60 * 60 * 1000)
             + (atoi(timestamp1[1]) * 30 * 24 * 60 * 60 * 1000)
             + (atoi(timestamp1[2]) * 24 * 60 * 60 * 1000)
             + (atoi(timestamp1[3]) * 60 * 60 * 1000)
             + (atoi(timestamp1[4]) * 60 * 1000)
             + (atoi(timestamp1[5]) * 1000)
             + atoi(timestamp1[6]);

    *totalDiff = total2 - total1;
}

/**
 * Split time string into timestamp array
 * @param date_token
 * @param hour_token
 * @param time
 * @param timestamp
 */
void splitTimestamp(char *date_token, char *hour_token, const char time[], char timestamp[][7])
{
    strncpy(date_token, time, 10);          // 9 characters (not include '\0') of date (ex: 2019-10-2323:21:45.638)
    strncpy(hour_token, (time+ 10), 12);    // next 12 characters (not include '\0') of hour

    int i = 0;
    char *date_el = strtok(date_token, "-");
    while (date_el != NULL)
    {
        strcpy(*(timestamp + i), date_el);
        i++;
        date_el = strtok(NULL, "-");
    }

    char *hour_el = strtok(hour_token, ":");
    while (hour_el != NULL)
    {
        strcpy(*(timestamp + i), hour_el);
        i++;
        hour_el = strtok(NULL, ":");
    }

    // Split millisecond with second
    char *sec_el = strtok(timestamp[5], ".");
    i--;
    while (sec_el != NULL)
    {
        strcpy(*(timestamp + i), sec_el);
        i++;
        sec_el = strtok(NULL, ".");
    }
}

char *multi_tok(char *input, char *delimiter)
{
    static char *string;
    if (input != NULL)
        string = input;

    if (string == NULL)
        return string;

    // Find delimiter
    char *end = strstr(string, delimiter);
    if (end == NULL)
    {
        char *temp = string;
        string = NULL;
        return temp;
    }

    char *temp = string;

    *end = '\0';
    string = end + strlen(delimiter); // move current string forward
    return temp;
}

/**
 * Parse separate time and code of each message to compare
 * @param i
 * @param time
 * @param code
 */
void parseMessageForTimeAndCode(int *i, char time[][30], char code[][100])
{
    char *token_msg = multi_tok(fileStr, "][");
    while (token_msg != NULL)
    {
        char* token_code = strstr(token_msg, "reqid");

        char* token_time = strtok(token_msg, "]");
        if (*(token_time) >= '0' && *(token_time) <= '9'
            && token_code != NULL)
        {
            strcpy(*(time + *i), token_time);
            strcpy(*(code + *i), token_code);
            ++*i;
        }

        token_msg = multi_tok(NULL, "][");
    }
}

/**
 * Average delay time
 * Find the messages that have the same "reqid" of sent and received messages (successful packages)
 * Then calculate average delay time from each total difference in the millisecond
 * @return
 */
int calculateAverageDelay()
{
    fileToStr(fileStr);

    int totalDelayTime = 0;
    int i = 0, totalSuccessPackages = 0;
    char code[19][100], time[19][30];
    int totalDiff;

    parseMessageForTimeAndCode(&i, time, code);

    processMessageCode(&i, code);

    for (int j = 0; j < i; j += 2)
    {
        // Compare successful sent and receive messages with the same code
        if (strcmp(*(code + j), *(code + j + 1)) == 0)
        {
            char timestamp1[7][7], timestamp2[7][7];  // 7 fields of time, max length 7 (second and millisecond - ex: 45.638)
            char date_token[11], hour_token[12];

            // Process date 1
            splitTimestamp(date_token, hour_token, *(time + j), timestamp1);

            // Process date 2
            splitTimestamp(date_token, hour_token, *(time + j + 1), timestamp2);

            calculateDiff(&totalDiff, timestamp1, timestamp2);
            totalDelayTime += totalDiff;
            ++totalSuccessPackages;
        }
    }
    return totalDelayTime / totalSuccessPackages;
}