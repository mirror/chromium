% Reuse default logic.
submit_rule(X) :-
    gerrit:default_submit(X),
    !.

% Special submit rule on Findit auto-reverts: A change is submittable if
% 1. If Findit created the change,
% 2. If Findit wants to commit the change,
% 3. If the change is a pure revert of another change.
submit_rule(submit(M, A, R)) :-
    change_is_on_master(M),
    created_and_committed_by_findit(A),
    commit_is_pure_revert(R).

% The change is on master branch.
change_is_on_master(M) :-
    gerrit:change_branch('refs/heads/master'),
    !,
    gerrit:commit_author(A),
    M = label('Change-On-Master', ok(A)).

change_is_on_master(M) :-
    gerrit:change_branch(B),
    B \= 'refs/heads/master',
    M = label('Change-On-Master', need(_)).

% Findit is CL author and now it tries to commit the CL.
created_and_committed_by_findit(A) :-
    gerrit:commit_author(Id, 'Findit', 'findit-for-me@appspot.gserviceaccount.com'),
    gerrit:current_user(Id),
    !,
    A = label('Authored-and-Committed-by-Findit', ok(Id)).

created_and_committed_by_findit(A) :-
    A = label('Authored-and-Committed-by-Findit', need(_)).

% This commit is pure revert of another commit.
commit_is_pure_revert(R) :-
    gerrit:pure_revert(1),
    !,
    gerrit:commit_author(A),
    R = label('Is-Pure-Revert', ok(A)).

commit_is_pure_revert(R) :-
    gerrit:pure_revert(U),
    U \= 1,
    R = label('Is-Pure-Revert', need(_)).