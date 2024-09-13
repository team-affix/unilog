% ROI listed here

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Helper

without_last([_], []) :- !.
without_last([X|Rest], [X|RestWithoutLast]) :-
    without_last(Rest, RestWithoutLast),
    !.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle Scoping

scope([Scope|RemainingScopes], [scope, Scope, InExp], OutExp) :-
    scope(RemainingScopes, InExp, OutExp),
    !.

scope([], Exp, Exp) :-
    !.

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

query(guide(GScope, Guide), theorem(TScope, TDescopedTheorem), conditions(Conds)) :-
    (
        unilog(guide(GScope, Guide), rule(theorem(TScope, TDescopedTheorem), conditions(Conds))),
        !
    )
    ;
    (
        % a fact will not have conditions, or a tscope
        unilog(guide(GScope, Guide), fact(theorem(RawTheorem))),
        scope(GScope, GScopedTheorem, RawTheorem),
        scope(TScope, GScopedTheorem, TDescopedTheorem),
        Conds = [],
        !
    )
    ;
    (
        unilog(guide(GScope, Guide), fact(guide(RecurGuide))),
        query(guide(GScope, RecurGuide), theorem(TScope, TDescopedTheorem), conditions(Conds)),
        !
    ).

query_all(guide(GScope, [G0|GuideRest]), theorem(TScope, [T0|TheoremRest]), conditions(Conds)) :-
    query(guide(GScope, G0), theorem(TScope, T0), conditions(CondsFirst)),
    query_all(guide(GScope, GuideRest), theorem(TScope, TheoremRest), conditions(CondsRest)),
    append(CondsFirst, CondsRest, Conds),
    !.

query_all(guide(_, []), theorem(_, []), conditions([])) :-
    !.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%% begin defining unilog predicate
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:- multifile unilog/2.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Begin built-ins (zero-arity)

unilog(guide(_, cons), rule(theorem(_, [cons, [X|Tail], X, Tail]), conditions([]))).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Begin ROI

unilog(guide(GScope, [tenter, EnteredTScope, NextGuide]), rule(theorem(CurrentTScope, [scope, EnteredTScope, TDescopedTheorem]), conditions(TScopedConds))) :-
    append(CurrentTScope, [EnteredTScope], NewTScope),
    query(guide(GScope, NextGuide), theorem(NewTScope, TDescopedTheorem), conditions(TDescopedConds)),
    scope_all([EnteredTScope], TScopedConds, TDescopedConds).

unilog(guide(GScope, [tleave, NextGuide]), rule(theorem(CurrentTScope, [descope, Thm]), conditions(Conds))) :-
    without_last(CurrentTScope, NewTScope),
    query(guide(GScope, NextGuide), theorem(NewTScope, Thm), conditions(Conds)).

unilog(guide(CurrentGScope, [genter, EnteredGScope, NextGuide]), rule(theorem(TScope, R), conditions(Conds))) :-
    append(CurrentGScope, [EnteredGScope], NewGScope),
    query(guide(NewGScope, NextGuide), theorem(TScope, R), conditions(Conds)).

unilog(guide(CurrentGScope, [gleave, NextGuide]), rule(theorem(TScope, R), conditions(Conds))) :-
    without_last(CurrentGScope, NewGScope),
    query(guide(NewGScope, NextGuide), theorem(TScope, R), conditions(Conds)).

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
