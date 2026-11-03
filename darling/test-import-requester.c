extern int authored_dependency_hidden(void);
static int (*volatile hidden_function)(void) = authored_dependency_hidden;

int authored_dependency_check(void) {
    return hidden_function == 0 ? 7 : 8;
}
