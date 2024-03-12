.. _roq-deribit:

.. |checkmark| unicode:: U+2713

roq-deribit
===========


Links
-----

* `Website <https://www.deribit.com/>`__
* `Test <https://test.deribit.com/>`__
* `Status <https://deribit.statuspage.io/>`__
* `Telegram <https://t.me/s/deribit_notifications>`__
* `Support <mailto:support@deribit.com>`__
* `Technical Support <mailto:dev@deribit.com>`__
* `Documentation <https://docs.deribit.com/v2/>`__
* `Deribit New London Infrastructure <https://www.deribit.com/pages/information/Londonsetup>`__


Purpose
-------

* Maintain network connectivity with the Deribit exchange
* Route exchange updates to connected clients
* Route client requests to the relevant exchange accounts
* Stream all messages to an event-log


Overview
--------

.. grid::  2
  :gutter: 2

  .. grid-item-card::  Products

    .. list-table::
      :widths: auto

      * - Spot
        -
      * - Futures
        - |checkmark|
      * - Options
        - |checkmark|

  .. grid-item-card::  Market Data

    .. list-table::
      :widths: auto

      * - Reference Data
        - |checkmark|
      * - Market Status
        - |checkmark|
      * - Top of Book
        - |checkmark|
      * - Market by Price (L2)
        - |checkmark|
      * - Market by Order (L3)
        -
      * - Trade Summary
        - |checkmark|
      * - Statistics
        - |checkmark|

  .. grid-item-card::  Order Management

    .. list-table::
      :widths: auto

      * - Create
        - |checkmark|
      * - Modify
        - |checkmark|
      * - Cancel
        - |checkmark|
      * - Cancel All
        - |checkmark|
      * - Auto Cancellation
        - |checkmark|

  .. grid-item-card::  Account Management

    .. list-table::
      :widths: auto

      * - Positions
        - |checkmark|
      * - Funds
        - |checkmark|

* Data center located in
  `LD4 <https://www.equinix.ch/locations/europe-colocation/united-kingdom-colocation/london-data-centers/ld4/>`__,
  `Equinix <https://www.equinix.com/>`__,
  Slough,
  UK


Conda
-----

* :ref:`Using Conda <tutorial-conda>`

.. tab:: Install

  .. code-block:: bash

    $ mamba install \
      --channel https://roq-trading.com/conda/stable \
      roq-deribit

.. tab:: Configure

  .. code-block:: bash

    $ cp $CONDA_PREFIX/share/roq-deribit/config.toml $CONFIG_FILE_PATH

    # Then modify $CONFIG_FILE_PATH to match your specific configuration

.. tab:: Run

  .. code-block:: bash

    $ roq-deribit \
          --name "deribit" \
          --config_file "$CONFIG_FILE_PATH" \
          --client_listen_address "$UNIX_SOCKET_PATH" \
          --service_listen_address "$TCP_LISTEN_PORT" \
          --flagfile "$FLAG_FILE"


Config
------

* :ref:`Common Config <gateway-config>`


Flags
-----

* :ref:`Using Flags <abseil-cpp>`
* :ref:`Common Flags <gateway-flags>`

.. code-block:: bash

   $ roq-deribit --help

.. tab:: Flags

   .. include:: flags/flags.rstinc

.. tab:: Common

   .. include:: flags/common.rstinc

.. tab:: FIX

   .. include:: flags/fix.rstinc

.. tab:: Multicast

   .. include:: flags/multicast.rstinc

.. tab:: WS

   .. include:: flags/ws.rstinc


Environments
------------

.. code-block:: bash

  $ $CONDA_PREFIX/share/roq-deribit/flags

.. tab:: Prod

   .. include:: flags/prod/flags.cfg
     :code: ini

.. tab:: Test

   .. include:: flags/test/flags.cfg
     :code: ini


Market Data
-----------

.. tab:: Live

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::ReferenceData`
      - MarketData
      - SecurityList (y)
      -

    * - :cpp:class:`roq::MarketStatus`
      - WebSocket
      - ticker
      -

    * - :cpp:class:`roq::TopOfBook`
      - WebSocket
      - quote
      -

    * - :cpp:class:`roq::MarketByPriceUpdate`
      - MarketData
      - MarketDataSnapshotFullRefresh (W), MarketDataIncrementalRefresh (X)
      -

    * - :cpp:class:`roq::MarketByOrderUpdate`
      -
      -
      - Unavailable

    * - :cpp:class:`roq::TradeSummary`
      - MarketData
      - MarketDataIncrementalRefresh (X)
      -

    * - :cpp:class:`roq::StatisticsUpdate`
      - MarketData
      - MarketDataIncrementalRefresh (X)
      -

.. tab:: Download

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::ReferenceData`
      - 
      - 
      -

    * - :cpp:class:`roq::MarketStatus`
      - 
      - 
      -

    * - :cpp:class:`roq::TopOfBook`
      -
      -
      -

    * - :cpp:class:`roq::MarketByPriceUpdate`
      - 
      - 
      - 

    * - :cpp:class:`roq::MarketByOrderUpdate`
      -
      -
      -

    * - :cpp:class:`roq::TradeSummary`
      - 
      - 
      - 

    * - :cpp:class:`roq::StatisticsUpdate`
      - 
      - 
      -


Statistics
~~~~~~~~~~

.. list-table::
  :header-rows: 1
  :widths: auto

  * - Type
    - Comments

  * - :cpp:class:`INDEX_VALUE`
    - Value of Index for INDEX instruments like BTC-DERIBIT-INDEX.
      MarketDataIncrementalRefresh (X) / MDEntryType (269) / Index Value (3).

  * - :cpp:class:`SETTLEMENT_PRICE`
    - Estimated Delivery Price for INDEX instruments like BTC-DERIBIT-INDEX
      MarketDataIncrementalRefresh (X) / MDEntryType (269) / Settlement Price (6).

  * - :cpp:class:`PRE_OPEN_INTEREST`
    - Open interest for the symbol.
      MarketDataIncrementalRefresh (X) / OpenInterest (790)

  * - :cpp:class:`PRE_SETTLEMENT_PRICE`
    - Mark price for the symbol.
      MarketDataIncrementalRefresh (X) / MarkPrice (100090)


Order Management
----------------

.. tab:: Live

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::OrderUpdate`
      - OrderEntry
      - ExecutionReport (8)
      -

    * - :cpp:class:`roq::TradeUpdate`
      - OrderEntry
      - ExecutionReport (8)
      -

.. tab:: Download

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::OrderUpdate`
      -
      -
      -

    * - :cpp:class:`roq::TradeUpdate`
      - DropCopy
      - private/get_user_trades_by_currency
      -

.. tab:: Request

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::CreateOrder`
      - OrderEntry
      - NewOrderSingle (D)
      -

    * - :cpp:class:`roq::ModifyOrder`
      - OrderEntry
      - OrderCancelReplaceRequest (G)
      -

    * - :cpp:class:`roq::CancelOrder`
      - OrderEntry
      - OrderCancelRequest (F)
      -

    * - :cpp:class:`roq::CancelAllOrders`
      - OrderEntry
      - OrderMassCancelRequest (q)
      -

.. tab:: Response

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::OrderAck`
      - OrderEntry
      - ExecutionReport (8), OrderCancelReject (9), Reject (3)
      -

Order Types
~~~~~~~~~~~

.. list-table::
  :header-rows: 1
  :widths: auto

  * - Type
    - Comments

  * - :cpp:class:`MARKET`
    - Mapped to :code:`'1'` (FIX)

  * - :cpp:class:`LIMIT`
    - Mapped to :code:`'2'` (FIX)


Time in Force
~~~~~~~~~~~~~

.. list-table::
  :header-rows: 1
  :widths: auto

  * - Type
    - Comments

  * - :cpp:class:`GTC`
    - Mapped to :code:`'1'` (FIX)

  * - :cpp:class:`IOC`
    - Mapped to :code:`'3'` (FIX)

  * - :cpp:class:`FOK`
    - Mapped to :code:`'4'` (FIX)



Position Effect
~~~~~~~~~~~~~~~

.. note::

  Not supported


Execution Instructions
~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
  :header-rows: 1
  :widths: auto

  * - Type
    - Comments

  * - :cpp:class:`PARTICIPATE_DO_NOT_INITIATE`
    - Mapped to :code:`'6'` (FIX)

  * - :cpp:class:`DO_NOT_INCREASE`
    - Mapped to :code:`'E'` (FIX)


Account Management
------------------

.. tab:: Live

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::PositionUpdate`
      - OrderEntry
      - RequestForPositions (AN) / PositionReport(AP)
      -

    * - :cpp:class:`roq::FundsUpdate`
      -
      -
      - Unavailable

.. tab:: Download

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::PositionUpdate`
      - OrderEntry
      - RequestForPositions (AN) / PositionReport(AP)
      -

    * - :cpp:class:`roq::FundsUpdate`
      - DropCopy
      - private/get_account_summary
      -


Streams
-------

.. tab:: OrderEntry

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Type
      - Comments

    * - FIX
      - Primary purpose

        * support order management

        Each connection

        * supports a single account


.. tab:: DropCopy

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Type
      - Comments

    * - WebSocket
      - Primary purpose

        * live account updates, including positions and funds

        Each connection

        * supports a single account

.. tab:: MarketData

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Type
      - Comments

    * - FIX
      - Primary purpose

        * live market data (everything else)

        Each connection

        * supports a slice of the symbols

        The master account is used to

        * authenticate, only

.. tab:: WebSocket

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Type
      - Comments

    * - WebSocket
      - Primary purpose

        * live market data (top of book + market status)

        Each connection

        * supports a slice of the symbols

        The first connection is used to

        * download currencies and symbols

.. tab:: Multicast

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Type
      - Comments

    * - UDP
      - Primary purpose

        * live market data (level 2 + top of book + market status)



Constraints
-----------

* The gateway requires a master account definition to be functional.
  This is needed by the FIX protocol, even for the market data connection.

* The field :code:`DeribitLabel` (FIX tag 100010) is limited to 64 characters

* The multicast feed can only be used by a single effective user id.
  This is a Linux restriction.

* The multicast protocol is flawed due to the snapshot channel containing no
  more than 10k levels (on either side) and the events channel including book
  updates for all levels.

  .. note::
     There are currently **no** work-arounds implemented to deal with this.

Comments
--------

* The gateway must be restarted at least daily if you use the multicast feed.
  The reason is the snapshot vs events inconsistency mentioned under the
  constraints.
  The book effectively becomes more and more *wrong* for big market moves.
