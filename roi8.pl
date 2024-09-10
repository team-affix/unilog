% ROI listed here

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle Scoping

scope([Scope|RemainingScopes], [scope, Scope, Exp], Descoped) :-
    scope(RemainingScopes, Exp, Descoped).

scope([], Exp, Exp).

scope_all(_, [], []) :-
    !.

scope_all(Scope, [TFirst | TRest], [DFirst | DRest]) :-
    scope(Scope, TFirst, DFirst),
    scope_all(Scope, TRest, DRest),
    !.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle querying

query_entry(Guide, DescopedTheorem) :-
    query([], Guide, DescopedTheorem, []).

query(Scope, Guide, DescopedTheorem, Conds) :-
    unilog(guide(Guide), rule(theorem(Scope, DescopedTheorem), conditions(Conds)))
    ;
    (
        % a fact will not have conditions, or a tscope
        unilog(guide(Guide), fact(theorem(Theorem))),
        scope(Scope, Theorem, DescopedTheorem),
        Conds = []
    ).

query_all(Scope, [G0|GuideRest], [T0|TheoremRest], Conds) :-
    query(Scope, G0, T0, CondsFirst),
    query_all(Scope, GuideRest, TheoremRest, CondsRest),
    append(CondsFirst, CondsRest, Conds),
    !.

query_all(_, [], [], []) :-
    !.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Begin built-ins (zero-arity)

unilog(guide(cons), rule(theorem(_, [cons, [X|Tail], X, Tail]), conditions([]))).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Begin ROI

unilog(guide([enter, EnteredScope, NextGuide]), rule(theorem(CurrentScope, [scope, EnteredScope, R]), conditions(ScopedConds))) :-
    append(CurrentScope, [EnteredScope], NewScope),
    query(NewScope, NextGuide, R, DescopedConds),
    scope_all([EnteredScope], ScopedConds, DescopedConds).

unilog(guide(cond), rule(theorem(_, Thm), conditions([Thm]))).

unilog(guide([discharge, Guide]), rule(theorem(Scope, [if, Thm, [and | Conds]]), conditions([]))) :-
    query(Scope, Guide, Thm, Conds).

unilog(guide([gor, FirstGuide | NextGuides]), rule(theorem(Scope, R), conditions(Conds))) :-
    query(Scope, FirstGuide, R, Conds);
    query(Scope, [gor | NextGuides], R, Conds).

unilog(guide([mp, G0, G1]), rule(theorem(Scope, YDescope), conditions(Conds))) :-
    query(Scope, G0, [if, YDescope, XDescope], ImpConds),
    query(Scope, G1, XDescope, AntConds),
    append(ImpConds, AntConds, Conds).

unilog(guide([conj]), rule(theorem(_, [and]), conditions([]))).

unilog(guide([conj | Guides]), rule(theorem(Scope, [and | DescopedTheorems]), conditions(Conds))) :-
    query_all(Scope, Guides, DescopedTheorems, Conds).

unilog(guide([simpl, Guide]), rule(theorem(Scope, Thm), conditions(Conds))) :-
    query(Scope, Guide, [and, Thm | _], Conds).

%%%%%%%%%%%%%%%%%%%%%%%
% GUIDES
%%%%%%%%%%%%%%%%%%%%%%%

unilog(guide(g_last), rule(theorem(Scope, R), conditions(Conds))) :-
    query(
        Scope,
        [gor,
            g_last_bc,
            [mp, g_last_gc, [conj, cons, g_last]]
        ],
        R,
        Conds
    ).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Theorems listed here.

unilog(guide(a0), fact(theorem([scope, m1, [if, y, x]]))).
unilog(guide(a1), fact(theorem([scope, m1, x]))).
unilog(guide(a2), fact(theorem([scope, m1, z]))).

unilog(
    guide(a3),
    fact(theorem(
        [scope, m1,
        [if,
            z,
            [cons, [a, b], a, [b]]
        ]
    ]))
).

unilog(
    guide(g_last_bc),
    fact(theorem([scope, alg, [last, [X], X]]))
).

unilog(
    guide(g_last_gc),
    fact(theorem(
        [scope, alg,
            [if,
                [last, L, X],
                [and,
                    [cons, L, _, T],
                    [last, T, X]
                ]
            ]
        ]
    ))
).
