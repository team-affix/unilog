#ifndef UNIT_TEST

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <SWI-Prolog.h>
#include "../CLI11/include/CLI/CLI.hpp"
#include "executor.hpp"

#define MAXLINE 1024

int main(int argc, char **argv)
{
    /* make the argument vector for Prolog */

    const char *plav[] = {argv[0], "--quiet", "--nosignals"};

    /* initialise Prolog */
    if (!PL_initialise(3, const_cast<char **>(plav)))
        PL_halt(1);

        /* Lookup calc/1 and make the arguments and call */

        // {
        //     predicate_t pred = PL_predicate("calc", 1, "user");

        //     term_t h0 = PL_new_term_refs(1);
        //     int rval;

        //     PL_put_atom_chars(h0, expression);
        //     rval = PL_call_predicate(NULL, PL_Q_NORMAL, pred, h0);

        //     PL_halt(rval ? 0 : 1);
        // }

#if 0

    // std::vector<std::string> l_input =
    //     {
    //         "uni",
    //         "--help",
    //     };
    std::vector<std::string> l_input =
        {
            "uni",
            "./src/test_input_files/executor_example_0/test.u",
            "./src/test_input_files/executor_example_1/main.u",
        };
    argc = l_input.size();
    std::vector<char *> l_input_converted;
    for (int i = 0; i < l_input.size(); i++)
    {
        l_input_converted.push_back(l_input[i].data());
    }
    argv = l_input_converted.data();
    // ------------------------------------------------

#endif

    CLI::App l_app("Unilog proof verifier", argv[0]);
    // l_app.require_subcommand(1);

    std::vector<std::string> l_files;
    l_app.add_option("files", l_files, "List of input files");

    using unilog::execute;
    using unilog::refer_statement;

    // PARSE ACTUAL DATA
    try
    {
        l_app.parse(argc, argv);

        // execute all unilog files
        for (const std::string &l_file : l_files)
        {
            std::cout << l_file << std::endl;

            try
            {
                execute(refer_statement{
                            .m_tag = make_atom("root"),
                            .m_file_path = make_atom(l_file),
                        },
                        make_nil());
            }
            catch (const std::runtime_error &l_err)
            {
                std::cout << l_err.what() << std::endl;
                exit(EXIT_FAILURE);
                // we must exit after first failure, since
                // cwd could be set to strange path, after exception is thrown
            }

            // clear the database before next file begins execution
            wipe_database();
        }
    }
    catch (const CLI::ParseError &e)
    {
        return l_app.exit(e);
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return 1;
    }

    PL_halt(0); // Properly halt the Prolog engine

    return 0;
}

#endif
