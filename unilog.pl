% ROI listed here

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Helper

without_last([_], []) :- !.
without_last([X|Rest], [X|RestWithoutLast]) :-
    without_last(Rest, RestWithoutLast),
    !.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle Scoping

universal([]).
universal(if).
universal(and).
universal(or).
universal(cons).
universal(believe).

% standard unification of two expressions
unify(Expr, Expr) :-
    !.
% empty list is a universal symbol
unify(Expr, Expr:_) :-
    universal(Expr),
    !.
unify(Expr:_, Expr) :-
    universal(Expr),
    !.
unify(Expr:_, Expr:_) :-
    universal(Expr),
    !.
% if both outermost expressions are lists, distribute scope then element-wise unify.
unify([XH|XT], [YH|YT]) :-
    !,
    unify(XH, YH),
    unify(XT, YT).
unify([XH|XT], [YH|YT]:S) :-
    !,
    unify(XH, YH:S),
    unify(XT, YT:S).
unify([XH|XT]:S, [YH|YT]) :-
    !,
    unify(XH:S, YH),
    unify(XT:S, YT).
unify([XH|XT]:S, [YH|YT]:S) :-
    !,
    unify(XH:S, YH:S),
    unify(XT:S, YT:S).

bscope(X, [S|NextBScope], Descoped) :-
    unify(X, [believe, S, Theorem]),
    bscope(Theorem, NextBScope, Descoped),
    !.
bscope(Theorem, [], Theorem) :-
    !.

rscope(X, RScope, Descoped) :-
    reverse(RScope, ReversedRScope),
    rscope_helper(X, ReversedRScope, Descoped).
rscope_helper(X, [S|NextRScope], Descoped) :-
    unify(X, RScoped:S),
    rscope_helper(RScoped, NextRScope, Descoped),
    !.
rscope_helper(Theorem, [], Theorem) :-
    !.

fully_qualify(Unqualified, RScope, BScope, Qualified) :-
    bscope(BQualified, BScope, Unqualified),
    bscope(Qualified, RScope, BQualified).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle querying

query(tag(RScope, GuideTag), theorem(BScope, Sexpr)) :-
    (
        query_fact(tag(RScope, GuideTag), theorem(BScope, Thm));
        unilog(tag(RScope, GuideTag), rule(theorem(Thm)));
        unilog(tag(RScope, GuideTag), guide(theorem(Thm)))
    ),
    (

    )
.

%unilog(tag(RScope, [mp, ImpGuide, JusGuide]), theorem(SScope, Symbol)) :-
%    unilog(tag(RScope, ImpGuide), theorem(SScope, ImpSExpr)),
%    unilog(tag(RScope, JusGuide), theorem(SScope, ImpSExpr)),
    

% example theorem
query_fact(tag(RScope, GuideTag), theorem(BScope, InSexpr)) :-
    unilog(tag(RScope, GuideTag), fact(theorem(FactSexpr))),
    bscope(FactSexpr, BScope, BDescoped),
    unify(BDescoped, InSexpr).
    
unilog(tag([], a0), fact(theorem([awesome, _]))).

unilog(tag([m1], a0), fact(theorem([awesome, _]:m1))).
