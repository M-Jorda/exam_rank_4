#include <stdio.h>
#include <malloc.h>
#include <ctype.h>

typedef struct node {
    enum {
        ADD,
        MULTI,
        VAL
    }   type;
    int val;
    struct node *l;
    struct node *r;
}   node;

// MODIF 1: Added forward declarations for recursive parsing functions
node *parse_value(char **s);
node *parse_mult(char **s);
node *parse_expr_r(char **s);

node    *new_node(node n)
{
    node *ret = calloc(1, sizeof(node));
    if (!ret)
        return (NULL);
    *ret = n;
    return (ret);
}

void    destroy_tree(node *n)
{
    if (!n)
        return ;
    if (n->type != VAL)
    {
        destroy_tree(n->l);
        destroy_tree(n->r);
    }
    free(n);
}

void    unexpected(char c)
{
    if (c)
        printf("Unexpected token '%c'\n", c);
    else
        printf("Unexpected end of input\n");
}

int accept(char **s, char c)
{
    if (**s == c)
    {
        (*s)++;
        return (1);
    }
    return (0);
}

int expect(char **s, char c)
{
    if (accept(s, c))
        return (1);
    unexpected(**s);
    return (0);
}

// MODIF 2: Added parse_value function to handle digits and parentheses
node *parse_value(char **s)
{
    if (isdigit(**s))
    {
        node n = {VAL, **s - '0', NULL, NULL};
        (*s)++;
        return new_node(n);
    }
    // MODIF 3: Using 'while' instead of 'if' to handle multiple consecutive opening parentheses
    while (accept(s, '('))
    {
        node *left = parse_expr_r(s);
        if (left == NULL)
            return NULL;
        if (!expect(s, ')'))
            return (destroy_tree(left), NULL);
        return left;
    }
    unexpected(**s);
    return NULL;
}

// MODIF 4: Added parse_mult function to handle multiplication operator
node *parse_mult(char **s)
{
    node *left = parse_value(s);
    if (!left)
        return NULL;
    while (accept(s, '*'))
    {
        node *right = parse_value(s);
        if (right == NULL)
            return (destroy_tree(left), NULL);
        node n = {MULTI, 0, left, right};
        left = new_node(n);
    }
    return left;
}

// MODIF 5: Added parse_expr_r function to handle addition operator
node *parse_expr_r(char **s)
{
    node *left = parse_mult(s);
    if (!left)
        return NULL;
    while (accept(s, '+'))
    {
        node *right = parse_mult(s);
        if (right == NULL)
            return (destroy_tree(left), NULL);
        node n = {ADD, 0, left, right};
        left = new_node(n);
    }
    return left;
}

// MODIF 6: Modified parse_expr to work with a local pointer copy
node    *parse_expr(char *s)
{
    char *p = s;
    node *ret = parse_expr_r(&p);
    if (ret == NULL)
        return NULL;
    // MODIF 7: Added unexpected() call before cleanup (was missing in source)
    if (*p) 
    {
        unexpected(*p);
        destroy_tree(ret);
        return (NULL);
    }
    return (ret);
}

int eval_tree(node *tree)
{
    switch (tree->type)
    {
        case ADD:
            return (eval_tree(tree->l) + eval_tree(tree->r));
        case MULTI:
            return (eval_tree(tree->l) * eval_tree(tree->r));
        case VAL:
            return (tree->val);
    }
    // MODIF 8: Added return 0 to avoid undefined behavior (compiler warning)
    return 0;
}

void run_tests(void);

int main(int argc, char **argv)
{
    // UNCOMMENT TO RUN TESTS:
    if (argc == 1) { run_tests(); return 0; }
    
    if (argc != 2)
        return (1);
    node *tree = parse_expr(argv[1]);
    if (!tree)
        return (1);
    printf("%d\n", eval_tree(tree));
    destroy_tree(tree);
}












// ========================================================================
// ======================== TEST FUNCTION - DO NOT LEARN ==================
// ========================================================================
#pragma region TEST_FUNCTION

void run_tests(void)
{
    struct test_case {
        char *input;
        int expected;
        int should_fail;
        char *description;
    } tests[] = {
        // Tests de base du sujet
        {"1", 1, 0, "Single digit"},
        {"2+3", 5, 0, "Simple addition"},
        {"3*4+5", 17, 0, "Mult then add (precedence)"},
        {"3+4*5", 23, 0, "Add and mult (precedence)"},
        {"(3+4)*5", 35, 0, "Parentheses change precedence"},
        {"(((((2+2)*2+2)*2+2)*2+2)*2+2)*2", 188, 0, "Complex nested parens"},
        
        // Tests d'additions/multiplications multiples
        {"1+2+3", 6, 0, "Multiple additions"},
        {"1+2+3+4+5", 15, 0, "Five additions"},
        {"2*3*4", 24, 0, "Multiple multiplications"},
        {"1+2*3+4", 11, 0, "Mixed operations"},
        
        // Tests de parenthèses
        {"(1)", 1, 0, "Simple parentheses"},
        {"(((((((3)))))))", 3, 0, "Multiple nested parentheses"},
        {"((1+2))", 3, 0, "Double parentheses"},
        {"(1+2)*3", 9, 0, "Parens with mult"},
        {"(1+2)*(3+4)", 21, 0, "Two parens groups"},
        {"((6*6+7+5+8)*(1+0+4*8+7)+2)+4*(1+2)", 2254, 0, "Complex expression from subject"},
        
        // Tests avec tous les chiffres (0-9)
        {"0", 0, 0, "Digit 0"},
        {"0+0", 0, 0, "Zero operations"},
        {"9", 9, 0, "Digit 9"},
        {"9*9", 81, 0, "Max single digit mult"},
        {"8*0", 0, 0, "Multiply by zero"},
        
        // Tests d'erreur - fin d'input inattendue
        {"1+", -1, 1, "ERROR: Unexpected end of input (missing operand)"},
        {"1*", -1, 1, "ERROR: Unexpected end of input (missing mult operand)"},
        {"(1+2", -1, 1, "ERROR: Unexpected end of input (missing closing paren)"},
        {"(", -1, 1, "ERROR: Unexpected end of input (paren not closed)"},
        {"((1)", -1, 1, "ERROR: Unexpected end of input (missing paren)"},
        
        // Tests d'erreur - token inattendu
        {"1+2)", -1, 1, "ERROR: Unexpected token ')' (extra closing paren)"},
        {")1+2", -1, 1, "ERROR: Unexpected token ')' (starts with closing paren)"},
        {"1++2", -1, 1, "ERROR: Unexpected token '+' (double operator)"},
        {"1**2", -1, 1, "ERROR: Unexpected token '*' (double operator)"},
        {"1 + 2", -1, 1, "ERROR: Unexpected token ' ' (space not handled)"},
        {"a", -1, 1, "ERROR: Unexpected token 'a' (letter)"},
        {"1a", -1, 1, "ERROR: Unexpected token 'a' (letter after digit)"},
        {"12", -1, 1, "ERROR: Unexpected token '2' (two digits - not allowed)"},
        {"1+23", -1, 1, "ERROR: Unexpected token '3' (two consecutive digits)"},
        {"((1+3)*12+(3*(2+6))", -1, 1, "ERROR: from subject - '12' causes error"},
        
        {NULL, 0, 0, NULL}
    };

    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║           VBC - RUNNING COMPREHENSIVE TESTS             ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    int passed = 0;
    int total = 0;
    int valid_tests = 0;
    int error_tests = 0;

    for (int i = 0; tests[i].input != NULL; i++)
    {
        total++;
        node *tree = parse_expr(tests[i].input);
        
        if (tests[i].should_fail)
        {
            // Test d'erreur - on s'attend à ce que parse_expr retourne NULL
            error_tests++;
            if (tree == NULL)
            {
                printf("✅ Test %2d PASSED: %s\n", i + 1, tests[i].description);
                printf("   Input: '%s' → Correctly failed\n\n", tests[i].input);
                passed++;
            }
            else
            {
                printf("❌ Test %2d FAILED: %s\n", i + 1, tests[i].description);
                printf("   Input: '%s'\n", tests[i].input);
                printf("   Expected: FAILURE, Got: %d (tree created)\n\n", eval_tree(tree));
                destroy_tree(tree);
            }
        }
        else
        {
            // Test valide - on s'attend à un résultat correct
            valid_tests++;
            if (!tree)
            {
                printf("❌ Test %2d FAILED: %s\n", i + 1, tests[i].description);
                printf("   Input: '%s' - Failed to parse (unexpected)\n\n", tests[i].input);
                continue;
            }
            
            int result = eval_tree(tree);
            if (result == tests[i].expected)
            {
                printf("✅ Test %2d PASSED: %s\n", i + 1, tests[i].description);
                printf("   Input: '%s' = %d\n\n", tests[i].input, result);
                passed++;
            }
            else
            {
                printf("❌ Test %2d FAILED: %s\n", i + 1, tests[i].description);
                printf("   Input: '%s'\n", tests[i].input);
                printf("   Expected: %d, Got: %d\n\n", tests[i].expected, result);
            }
            destroy_tree(tree);
        }
    }

    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║                       RESULTS                            ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  Total Tests:      %2d                                   ║\n", total);
    printf("║  Valid Tests:      %2d                                   ║\n", valid_tests);
    printf("║  Error Tests:      %2d                                   ║\n", error_tests);
    printf("║  Passed:           %2d / %2d                             ║\n", passed, total);
    printf("║  Failed:           %2d / %2d                             ║\n", total - passed, total);
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    if (passed == total)
        printf("🎉 ALL TESTS PASSED! 🎉\n\n");
    else
        printf("⚠️  Some tests failed. Review the output above.\n\n");
}

#pragma endregion
// ========================================================================
// ====================== END OF TEST FUNCTION ============================
// ========================================================================
