:- multifile unilog/2.

%%%%%%%%%%%%%%%%%%%%%%%
% GUIDES
%%%%%%%%%%%%%%%%%%%%%%%

unilog(guide([], g_last), rule(theorem(TScope, R), conditions(Conds))) :-
    query(
        guide(
            [],
            [gor,
                g_last_bc,
                [mp, g_last_gc, [conj, cons, g_last]]
            ]
        ),
        theorem(
            TScope,
            R
        ),
        conditions(
            Conds
        )
    ).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Theorems listed here.

unilog(guide([], a0), fact(theorem([scope, m1, [if, y, x]]))).
unilog(guide([], a1), fact(theorem([scope, m1, x]))).
unilog(guide([], a2), fact(theorem([scope, m1, z]))).

unilog(
    guide([], a3),
    fact(
        theorem(
            [scope, m1,
                [if,
                    z,
                    [cons, [a, b], a, [b]]
                ]
            ]
        )
    )
).

unilog(
    guide([], g_last_bc),
    fact(theorem([scope, alg, [last, [X], X]]))
).

unilog(
    guide([], g_last_gc),
    fact(
        theorem(
            [scope, alg,
                [if,
                    [last, L, X],
                    [and,
                        [cons, L, _, T],
                        [last, T, X]
                    ]
                ]
            ]
        )
    )
).

unilog(guide([m2], z0), fact(theorem([if, y, x]))).
unilog(guide([m2], z1), fact(theorem(x))).
unilog(guide([m2], z2), fact(theorem(z))).
