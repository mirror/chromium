# Merge Request Process
[TOC]

## **Introduction**


This document formally defines Chrome’s merge request and approval process.
Given Chrome’s aggressive 6 week release schedule and commitment to the 4 S’s
(speed, stability, security, and simplicity), it’s important to have a merge
process that helps us meet our highest standards.

Please read overview of Chrome Release Cycle to understand in detail how the
Chrome release cycle works and understand key release concepts and terminology.
Please read Defining Release Blockers to understand how issues/bugs are
categorized as release blocking. List of schedule and release owners can be
found at go/chrome-schedule


##**When to Request a Merge?**
Chrome ships across multiple different platforms and OS’s. It’s important to
ensure that all features and changes are first landed in trunk, tested in
canary and dev, and then tested in beta. Once fix is verified in beta, then it
should be promoted to stable. However, due to many reasons and scenarios, it’s
possible that changes may miss branch date and require a merge post branch.

**Merge**: any change that is cherry picked to a branch post branch point.
This section will discuss when a merge should be requested, depending on
timeframe and criticality of the bug. Please note that the scrutiny of merges
gets higher as we get closer to the stable launch date. Merges post
stable-rollout have a higher bar than merges prior to Stable.

***Phase 1: First Two Weeks After Branch (two weeks before beta promotion)***

There are bugs and polish fixes, that may not necessarily be considered
critical, but help with the overall quality of the product. There are also
scenarios where dependent CL’s are missed by hours or days. To accommodate
these scenarios, merges will be considered for all polish bugs, regressions,
and release blocking bugs. Please note that merges will not be accepted for
implementing or enabling brand new features or functionality. Features
should already be complete. Merges will be reviewed manually and
automatically, depending on the type of change.

GRD file changes are only allowed during this phase. If you have a critical 
string change needed after this phase, please reach out to release owner or
TPMs.

Please note that there will be two or three Dev release cycles during
branch stabilization period.
      
***Phase 2: First Four Weeks of Beta Rollout***

We stabilize the branch for two weeks after branch point (branch
stabilization period) in order to ship Beta. Beta starts after branch
stabilization and lasts up to five weeks. Stable release begins during
the 5th Beta week and there can be one week overlap between beta releases
and stable.
        
During the first four weeks of Beta, merges should only be requested if
the bug is considered either release blocking (stable, beta, or dev) or
considered a high-impact regression. (Defining Release Blockers)
         
Security bugs should be consulted with chrome-security-tpms@ to
determine criticality. If your issue does not meet the above criteria
but you would still like to consider this merge, please reach out to
release owner or TPMs.
          
 ***Phase 3: Last Two Weeks of Beta and Post Stable***

During the last 2 weeks of Beta and after stable promotion, merges
should only be requested for critical bugs marked as either RB-Dev,
RB-Beta or RB-Stable, with minimal code complexity. For these merges,
please add detailed explanation why a merge is requested. If a merge
is accepted, this will be a candidate for postmortem. 
           
Security bugs should be consulted with [chrome-security@](chrome-security@google.com)
to determine criticality.

![Chrome Merge Schedule](../images/chrome_merge_schedule.png)

Key Event  | Date
------- | --------
Feature Freeze | June 23rd, 2017
Branch Date | July 20th, 2017
Branch Stabilization Period | July 20th to August 3rd, 2017
Merge Reviews Phase 1 | July 20th to August 3rd, 2017
Beta Rollout | August 3rd to September 12th 2017
Merge Reviews Phase 2 | August 3rd to August 31st 2017
Merge Reviews Phase 3 | August 31st 2017 and post Stable release
Stable Release | September 6th, 2017 + rollout schedule

##**Merge Requirements**
Before requesting a merge, please ensure following conditions are met
and please provide full justification for merge request in CRBug:
*   **Full automated unit test coverage:** please add unit tests or
    functional tests before requesting a merge.

*   **Deployed in Canary for at least 24 hours:** change has to
    be tested and verified in Canary or Dev, before requesting a
    merge. Fix should be tested by either test engineering or the
    original reporter of the bug. 

*   **Safe Merge:** Need full developer confidence that the
    change will be a safe merge. Safe merge means that your
    change will not introduce new regressions, or break
    anything existing. If you are not confident that your
    merge is fully safe, then reach out to TL or TPMs for
    guidance.

##**Merge Request**
If the merge review requirements are met (listed in
section above) and your change fits one of the timelines
above, please go ahead and apply the
Merge-Request-[Milestone Number] label on the bug and
please provide justification in the bug.

Once Merge is approved, bug will be marked with
Merge-Approved-[Milestone-Number] label. Please merge
**immediately after**. Please note that if change is not
merged in time after approval, it can be rejected.

If merge is rejected, “Merge-Rejected” label will be
applied. If you think it’s important to consider the
change for merge and does not meet the criteria above,
please reach out to the release owners, TPMs or TLs for
guidance. 

##**Merge Reviews**

The release team has an automated process that helps
with the merge evaluation process. It will enforce many
of the rules listed in sections above. If the rules
above don’t pass, it will either auto-reject or flag
for manual review. Please allow up to 24 hours for the
automated process to take effect.

Manual merge reviews will be performed by release
owners and TPMs. 

##**Contacts**
Please check go/chrome-schedule to see current release
owners. For more details, please reach out to the
following folks depending on platform and type of
changes.

*  Desktop: abdulsyed@, govind@
*  Android: amineer@
*  iOS: cmasso@
*  ChromeOS: ketakid@, bhthompson@, gkihumba@, josafat@
*  For other questions: amineer@
