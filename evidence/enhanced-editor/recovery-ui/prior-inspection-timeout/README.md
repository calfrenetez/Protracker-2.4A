# Visual inspection timeout

Run recovery-ui-1790555432596880000 failed before sending Recover input because
a single second-launch capture showed the transient black frame. The first
requester was visibly inspected and Keep current passed. The timeout path
closed the second requester/editor normally, restored all5 ENV values and their
presence exactly, and passed DMAoff and independent cleanup/absence checks.
There was no reset/restart or physical operation. The runner now refreshes
captures during its bounded inspection gate. This failed run is not UI acceptance.
