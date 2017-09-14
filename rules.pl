% If Findit wants to commit a change it created, special submit rule on
% Findit auto-reverts should be used: The change is submittable only if
% 3. The change is on master branch,
% 4. The change is a pure revert of another change.
submit_rule(submit(M, R)) :-
    created_and_committed_by_findit(),
    !,
    change_is_on_master(M),
    commit_is_pure_revert(R).

% Otherwise use default submit rule.
submit_rule(X) :-
  \+ created_and_committed_by_findit(),
  gerrit:default_submit(X).

% Findit is CL author and now it tries to commit the CL.
created_and_committed_by_findit() :-
    gerrit:commit_author(Id, 'Findit', 'findit-for-me@appspot.gserviceaccount.com'),
    gerrit:current_user(Id).

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