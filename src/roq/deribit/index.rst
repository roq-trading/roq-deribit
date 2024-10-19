.. _roq-deribit:

.. |checkmark| unicode:: U+2713

roq-deribit
===========

.. tab:: Stable

  .. code-block:: shell

     $ mamba install \
           --channel https://roq-trading.com/conda/stable \
           roq-deribit

.. tab:: Unstable

  .. code-block:: shell

     $ mamba install \
           --channel https://roq-trading.com/conda/unstable \
           roq-deribit


:code:`roq-deribit`
-------------------

.. code-block:: shell

   $ roq-deribit [FLAGS]


Description
~~~~~~~~~~~

:code:`roq-deribit` is a gateway


Supports
~~~~~~~~

.. grid::  2
  :gutter: 2

  .. grid-item-card::  Products

    .. list-table::
      :widths: auto

      * - Spot
        - ?
      * - Futures
        - |checkmark|
      * - Options
        - |checkmark|
      * - Combos
        - ?

  .. grid-item-card::  Market Data

    .. list-table::
      :widths: auto

      * - Reference Data
        - |checkmark|
      * - Market Status
        - |checkmark|
      * - Top of Book
        - |checkmark|
      * - Market by Price
        - |checkmark|
      * - Market by Order
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
      * - Auto-Cancel
        - |checkmark|

  .. grid-item-card::  Account Management

    .. list-table::
      :widths: auto

      * - Positions
        - |checkmark|
      * - Funds
        - |checkmark|


.. _roq-deribit-flags:

Flags
~~~~~

.. code-block:: shell

   $ roq-deribit --help

.. tab:: Flags

   .. include:: flags/flags.rstinc

.. tab:: FIX

   .. include:: flags/fix.rstinc

.. tab:: WS

   .. include:: flags/ws.rstinc

.. tab:: Multicast

   .. include:: flags/multicast.rstinc

.. tab:: Download

   .. include:: flags/download.rstinc

.. tab:: MBP

   .. include:: flags/mbp.rstinc

.. tab:: Request

   .. include:: flags/request.rstinc

.. tab:: Misc

   .. include:: flags/misc.rstinc


Environments
~~~~~~~~~~~~

.. tab:: Prod

   .. code-block:: shell

      $ $CONDA_PREFIX/share/roq-deribit/flags/prod/flags.cfg

   .. include:: flags/prod/flags.cfg
     :code: ini

.. tab:: Test

   .. code-block:: shell

      $ $CONDA_PREFIX/share/roq-deribit/flags/test/flags.cfg

   .. include:: flags/test/flags.cfg
     :code: ini


Configuration
~~~~~~~~~~~~~

.. code-block:: shell

   $ $CONDA_PREFIX/share/roq-deribit/config.toml

.. important::

   The template will be replaced when the software is upgraded.
   Make a copy and modify to your needs.

.. include:: config.toml
   :code: toml


Market Data
~~~~~~~~~~~

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
^^^^^^^^^^

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
~~~~~~~~~~~~~~~~

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
^^^^^^^^^^^

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
^^^^^^^^^^^^^

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
^^^^^^^^^^^^^^^

.. note::

  Not supported


Execution Instructions
^^^^^^^^^^^^^^^^^^^^^^

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
~~~~~~~~~~~~~~~~~~

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
~~~~~~~

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
~~~~~~~~~~~

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
~~~~~~~~

* The gateway must be restarted at least daily if you use the multicast feed.
  The reason is the snapshot vs events inconsistency mentioned under the
  constraints.
  The book effectively becomes more and more *wrong* for big market moves.


:code:`roq-deribit-filter`
--------------------------

.. code-block:: shell

   $ roq-deribit-filter [FLAGS]


Description
~~~~~~~~~~~

:code:`roq-deribit-filter` is a tool to generate the PCAP filter required to capture specific channels.


Flags
~~~~~

.. code-block:: shell

   $ roq-deribit-filter --help

.. tab:: Flags

   .. include:: filter/flags/flags.rstinc

.. tab:: Multicast

   .. include:: filter/flags/multicast.rstinc


Example
~~~~~~~

.. code-block:: shell

   $ roq-deribit-filter \
       --type "tcpdump" \
       --multicast_channel_ids 1,2 \
       --multicast_config_file "$CONDA_PREFIX/share/roq-deribit/prod/channels.json"

   (port 6100 or port 6101) and (host 239.111.111.1 or host 239.111.111.2)


This will output a :code:`tcpdump` filter for :code:`channel_ids`.


References
----------

Common
~~~~~~

* :ref:`Using Conda <tutorial-conda>`
* :ref:`Using Flags <abseil-cpp>`
* :ref:`Gateway Flags <gateway-flags>`
* :ref:`Gateway Config <gateway-config>`

Deribit
~~~~~~~

* `Website <https://www.deribit.com/>`__
* `Test <https://test.deribit.com/>`__
* `Status <https://deribit.statuspage.io/>`__
* `Telegram <https://t.me/s/deribit_notifications>`__
* `Support <mailto:support@deribit.com>`__
* `Technical Support <mailto:dev@deribit.com>`__
* `Documentation <https://docs.deribit.com/v2/>`__
* `Deribit New London Infrastructure <https://www.deribit.com/pages/information/Londonsetup>`__
