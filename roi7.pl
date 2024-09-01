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
    roi(Scope, Guide, DescopedTheorem, Conds)
    ;
    (
        theorem(Guide, Theorem),
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

roi(_, cons, [cons, [X|Tail], X, Tail], []).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Begin ROI

roi(CurrentScope, [enter, EnteredScope, NextGuide], [scope, EnteredScope, R], ScopedConds) :-
    append(CurrentScope, [EnteredScope], NewScope),
    query(NewScope, NextGuide, R, DescopedConds),
    scope_all([EnteredScope], ScopedConds, DescopedConds).

roi(_, cond, Thm, [Thm]).

roi(Scope, [discharge, Guide], [if, Thm, [and | Conds]], []) :-
    query(Scope, Guide, Thm, Conds).

roi(Scope, [gor, FirstGuide | NextGuides], R, Conds) :-
    query(Scope, FirstGuide, R, Conds);
    query(Scope, [gor | NextGuides], R, Conds).

roi(Scope, [mp, G0, G1], YDescope, Conds) :-
    query(Scope, G0, [if, YDescope, XDescope], ImpConds),
    query(Scope, G1, XDescope, AntConds),
    append(ImpConds, AntConds, Conds).

roi(_, [conj], [and], []).

roi(Scope, [conj | Guides], [and | DescopedTheorems], Conds) :-
    query_all(Scope, Guides, DescopedTheorems, Conds).

roi(Scope, [simpl, Guide], Thm, Conds) :-
    query(Scope, Guide, [and, Thm | _], Conds).

roi(Scope, [f_iffdef, Guide], [iff, X, Y], Conds) :-
    query(Scope, Guide, [and, [if, X, Y], [if, Y, X]], Conds).

roi(Scope, [b_iffdef, Guide], [and, [if, X, Y], [if, Y, X]], Conds) :-
    query(Scope, Guide, [iff, X, Y], Conds).

roi(Scope, [commute_and, Guide], [and, Y, X], Conds) :-
    query(Scope, Guide, [and, X, Y], Conds).

%%%%%%%%%%%%%%%%%%%%%%%
% GUIDES
%%%%%%%%%%%%%%%%%%%%%%%

roi(Scope, g_last, R, Conds) :-
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

theorem(a0, [scope, m1, [if, y, x]]).
theorem(a1, [scope, m1, x]).
theorem(a2, [scope, m1, z]).

theorem(and_fxn_0,
    [iff,
        [and_fxn, A, B],
        [and, A, B]
    ]
).

theorem(a3,
[scope, m1,
    [if,
        z,
        [cons, [a, b], a, [b]]
    ]
]).

theorem(
    g_last_bc,
    [scope, alg, [last, [X], X]]
).

theorem(
    g_last_gc,
    [scope, alg,
        [if,
            [last, L, X],
            [and,
                [cons, L, _, T],
                [last, T, X]
            ]
        ]
    ]
).

