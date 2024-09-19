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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle querying

%unilog(tag(RScope, [mp, ImpGuide, JusGuide]), theorem(SScope, Symbol)) :-
%    unilog(tag(RScope, ImpGuide), theorem(SScope, ImpSExpr)),
%    unilog(tag(RScope, JusGuide), theorem(SScope, ImpSExpr)),
    
