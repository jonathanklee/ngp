#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { char *message = test(); tests_run++; \
                               if (message) return message; } while (0)
extern int tests_run;


#define mu_assert_verbose(test) \
    do { \
        if (!(test)) { \
            printf("\033[31;1mFAILED:\033[0m %s:%d -> %s", __FUNCTION__, __LINE__, #test); \
            return ""; \
        } \
    } while (0)
