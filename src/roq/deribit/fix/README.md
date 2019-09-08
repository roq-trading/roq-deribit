
ExecutionReport

1362	NoFills	No	Number of immediate fill entries for the order
=>1363	FillExecID	No	Unique identifier of execution, concatenated via '#' symbol and trade sequence number, e.g., BTC-28SEP18#38.
=>1364	FillPx	No	Price of this partial fill
=>1365	FillQty

52	SendingTime	Yes

MassStatusReqType=7



OrderMassCancelRequest
OrderMassCancelReport

OrderMassStatusRequest --> Nx ExecutionReport
