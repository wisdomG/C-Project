### 基于线程池的 http web 服务器

#### 字符串处理函数
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
strcasecmp: 比较字符串s1和s2，比较时忽略大小写，s1=s2时返回0，s1>s2时返回大于0的值，s1<s2时返回小于0的值，即第一个不相等的字符之差。
strncasecmp: 比较字符串s1和s2的前n个字符，返回值同上

size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
strspn: 计算字符串s中连续多少个字符串属于accept，从s中的第一个字符开始查找，如果第一个字符不属于accept，则直接返回0
strcspn: 计算字符串s中连续多少个字符都不属于accept，查找方式同上

char *strpbrk(const char *s, const char *accept); 
strpbrk: 返回字符串s中最先含有字符串accept中任一字符的位置，找不到则返回NULL

char *strchr(const char* s, int c);
char *strrchr(const char* s, int c);
strchr: 返回字符串s中首次出现字符c的位置
strrchr: 返回字符串s中最后一次出现字符c的位置

char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
strcpy: 将src指向的字符串拷贝到dest指向的数组中，并返回复制后的dest
strncpy: 将src指向的字符串的前n个字符拷贝到dest指向的数组中，并返回复制后的dest
