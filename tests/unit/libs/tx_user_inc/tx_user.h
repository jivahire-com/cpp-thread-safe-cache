//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

#ifndef TX_USER_H
#define TX_USER_H

#ifndef __ASSEMBLER__

#define TX_THREAD_USER_EXTENSION  \
    ULONG tx_thread_execution_time_last_ready; \
    ULONG tx_thread_execution_time_last_start; \
    ULONG tx_thread_execution_time_total; \
    UINT  rtosmon_id; \
    VOID  *tx_fasthisto_ptr; \
    UINT  tx_last_state;

#define TX_ENABLE_EVENT_LOGGING

#endif /* __ASSEMBLER__ */
#endif /* TX_USER_H */