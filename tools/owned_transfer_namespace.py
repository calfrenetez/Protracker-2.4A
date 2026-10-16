"""Once-only fresh RAM namespace for a separately observed external transfer.

No connection, FTP client, upload, native launch, delete or recovery is provided.
The caller supplies live target/ownership guards and durable observations, then
performs the actual transfer. Matching existing content never grants ownership.
Both the preparation and whole lifetime begin before the first remote request.
"""
import asyncio
import math
import re
import time

import native_file_protocol as wire
from owned_native_input import InputPlan
from owned_native_run import require


class TransferNamespace:
    def __init__(self, plan, exchange, record, *, download_name, clock=time.monotonic,
                 seconds=250, preparation_seconds=120):
        require(type(plan) is InputPlan and plan.external_transfer,
                'Explicit external input plan required')
        require(all(callable(f) for f in (exchange, record, clock)), 'Transfer callbacks required')
        # Use the receipt's filename grammar without constructing an ownership claim.
        require(type(download_name) is str and re.fullmatch(r'[A-Za-z0-9_-]{1,30}', download_name),
                'Exact flat transfer filename required')
        require(download_name.lower() not in {plan.directory.split(':')[1].lower(),
                plan.execution.directory.split(':')[1].lower(), 'nativetest'},
                'Download filename overlaps an owned namespace')
        require(type(seconds) in (int, float) and math.isfinite(seconds) and 0 < seconds <= 250
                and type(preparation_seconds) in (int, float) and math.isfinite(preparation_seconds)
                and 0 < preparation_seconds <= 120, 'Bounded transfer lifetime required')
        self.plan, self.exchange, self.record = plan, exchange, record
        self.download_name, self.clock = download_name, clock
        started = clock()
        self.deadline = started + seconds
        self.prepare_deadline = min(self.deadline, started + preparation_seconds)
        self.phase, self.calls = 'NEW', 0

    def remaining(self):
        value = min(self.deadline, self.prepare_deadline) - self.clock()
        require(value > 0, 'External transfer preparation deadline expired')
        return value

    async def _ask(self, command, reply_type, seconds):
        require(self.phase == 'PREPARING' and self.calls < 128, 'Namespace preparation is inactive or exhausted')
        self.calls += 1
        budget = min(seconds, self.remaining())
        self.record({'event': 'external_namespace_dispatch', 'number': self.calls, 'command': command})
        try:
            raw = await asyncio.wait_for(self.exchange(command, reply_type, budget), budget)
        except wire.BridgeReplyError as error:
            self.record({'event': 'external_namespace_error_reply', 'number': self.calls, 'raw': error.raw})
            raise
        self.remaining()
        wire.line(raw)
        self.record({'event': 'external_namespace_reply', 'number': self.calls, 'raw': raw})
        return raw

    async def prepare(self):
        require(self.phase == 'NEW', 'External namespace cannot be replayed')
        self.phase = 'PREPARING'
        try:
            self.record({'event': 'external_namespace_plan', 'plan': self.plan.description(),
                         'downloadName': self.download_name})
            entries = await wire.Observation(self._ask, seconds=min(20, self.remaining()),
                                              clock=self.clock).directory('RAM:')
            wire.absent(entries, (self.plan.execution.directory.split(':')[1],
                                  self.plan.directory.split(':')[1], self.download_name))
            raw = await self._ask({'type': 'MAKEDIR', 'path': self.plan.directory}, 'OK', 4)
            require(raw == 'OK|MAKEDIR|' + self.plan.directory, 'Fresh external namespace creation unconfirmed')
            entries = await wire.Observation(self._ask, seconds=min(20, self.remaining()),
                                              clock=self.clock).directory(self.plan.directory)
            require(entries == (), 'Fresh external namespace is not empty')
            self.remaining()
            result = {'status': 'FRESH_NAMESPACE_WAITING_EXTERNAL_TRANSFER',
                      'plan': self.plan.description(), 'downloadName': self.download_name}
            self.record({'event': 'external_namespace_ready', **result})
            self.phase = 'WAITING_TRANSFER'
            return result
        except BaseException:
            self.phase = 'FAILED'
            raise

    def claim(self):
        """Claim once before adoption; this does not verify or perform a transfer.

        The service must still require ReceiptObserver and independent input
        custody. It caps both new deadlines using these returned original bounds.
        No claim is usable for another target or as an execution permission.
        """
        require(self.phase == 'WAITING_TRANSFER', 'Prepared external namespace required once')
        try:
            self.remaining()
            self.phase = 'CLAIMED'
            return {'deadline': self.deadline, 'prepareDeadline': self.prepare_deadline}
        except BaseException:
            self.phase = 'FAILED'
            raise
