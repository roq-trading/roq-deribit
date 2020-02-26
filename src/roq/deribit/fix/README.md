
There's no core functionality to support missing values
RequestForPositions
	{ "tag": 15, "name": "Currency", "type": "std::string_view" }

Online documentation has wrong key field
MDFull
	{ "tag": 269, "name": "MDEntryType", "type": "roq::core::fix::MDEntryType" },




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
