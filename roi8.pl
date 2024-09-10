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
    query(guide([], Guide), theorem([], DescopedTheorem), conditions([])).

query(guide(GScope, Guide), theorem(TScope, DescopedTheorem), conditions(Conds)) :-
    unilog(guide(GScope, Guide), rule(theorem(TScope, DescopedTheorem), conditions(Conds)))
    ;
    (
        % a fact will not have conditions, or a tscope
        unilog(guide(GScope, Guide), fact(theorem(Theorem))),
        scope(TScope, Theorem, DescopedTheorem),
        Conds = []
    ).

query_all(guide(GScope, [G0|GuideRest]), theorem(TScope, [T0|TheoremRest]), conditions(Conds)) :-
    query(guide(GScope, G0), theorem(TScope, T0), conditions(CondsFirst)),
    query_all(guide(GScope, GuideRest), theorem(TScope, TheoremRest), conditions(CondsRest)),
    append(CondsFirst, CondsRest, Conds),
    !.

query_all(guide(_, []), theorem(_, []), conditions([])) :-
    !.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Begin built-ins (zero-arity)

unilog(guide(_, cons), rule(theorem(_, [cons, [X|Tail], X, Tail]), conditions([]))).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Begin ROI

unilog(guide(GScope, [tenter, EnteredScope, NextGuide]), rule(theorem(CurrentTScope, [scope, EnteredScope, R]), conditions(ScopedConds))) :-
    append(CurrentTScope, [EnteredScope], NewTScope),
    query(guide(GScope, NextGuide), theorem(NewTScope, R), conditions(DescopedConds)),
    scope_all([EnteredScope], ScopedConds, DescopedConds).

unilog(guide(CurrentGScope, [genter, EnteredScope, NextGuide]), rule(theorem([EnteredScope|TScopeRest], R), conditions(Conds))) :-
    append(CurrentGScope, [EnteredScope], NewGScope),
    query(guide(NewGScope, NextGuide), theorem(TScopeRest, R), conditions(Conds)).

unilog(guide(_, cond), rule(theorem(_, Thm), conditions([Thm]))).

unilog(guide(GScope, [discharge, Guide]), rule(theorem(TScope, [if, Thm, [and | Conds]]), conditions([]))) :-
    query(guide(GScope, Guide), theorem(TScope, Thm), conditions(Conds)).

unilog(guide(GScope, [gor, FirstGuide | NextGuides]), rule(theorem(TScope, R), conditions(Conds))) :-
    query(guide(GScope, FirstGuide), theorem(TScope, R), conditions(Conds));
    query(guide(GScope, [gor | NextGuides]), theorem(TScope, R), conditions(Conds)).

unilog(guide(GScope, [mp, G0, G1]), rule(theorem(TScope, YDescope), conditions(Conds))) :-
    query(guide(GScope, G0), theorem(TScope, [if, YDescope, XDescope]), conditions(ImpConds)),
    query(guide(GScope, G1), theorem(TScope, XDescope), conditions(AntConds)),
    append(ImpConds, AntConds, Conds).

unilog(guide(_, [conj]), rule(theorem(_, [and]), conditions([]))).

unilog(guide(GScope, [conj | Guides]), rule(theorem(TScope, [and | DescopedTheorems]), conditions(Conds))) :-
    query_all(guide(GScope, Guides), theorem(TScope, DescopedTheorems), conditions(Conds)).

unilog(guide(GScope, [simpl, Guide]), rule(theorem(TScope, Thm), conditions(Conds))) :-
    query(guide(GScope, Guide), theorem(TScope, [and, Thm | _]), conditions(Conds)).

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
