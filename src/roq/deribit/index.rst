.. _roq-deribit:

.. |dagger| unicode:: U+2020
.. |double-dagger| unicode:: U+2021
.. |right-arrow| unicode:: U+2192
.. |right-double-arrow| unicode:: U+21D2
.. |left-right-double-arrow| unicode:: U+21D4
.. |check-mark| unicode:: U+2705
.. |cross-mark| unicode:: U+274C
.. |negative-cross-mark| unicode:: U+274E
.. |footnote-1| unicode:: U+2776
.. |footnote-2| unicode:: U+2777
.. |footnote-3| unicode:: U+2778


roq-deribit
===========

.. tab:: Unstable

  .. code-block:: shell

     $ conda install \
           --channel https://roq-trading.com/conda/unstable \
           roq-deribit

.. tab:: Stable

  .. code-block:: shell

     $ conda install \
           --channel https://roq-trading.com/conda/stable \
           roq-deribit


Supports
--------

.. grid::  2
  :gutter: 2

  .. grid-item-card::  Products

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:enumerator:`Spot <roq::SecurityType::SPOT>`
        - |check-mark|
        -
      * - :cpp:enumerator:`Futures <roq::SecurityType::FUTURES>`
        - |check-mark|
        -
      * - :cpp:enumerator:`Swap <roq::SecurityType::SWAP>`
        - |check-mark|
        -
      * - :cpp:enumerator:`Option <roq::SecurityType::OPTION>`
        - |check-mark|
        -

  .. grid-item-card::  Market Data

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:class:`ReferenceData <roq::ReferenceData>`
        - |check-mark|
        -
      * - :cpp:class:`MarketStatus <roq::MarketStatus>`
        - |check-mark|
        -
      * - :cpp:class:`TopOfBook <roq::TopOfBook>`
        - |check-mark|
        -
      * - :cpp:class:`MarketByPrice <roq::MarketByPriceUpdate>`
        - |check-mark|
        -
      * - :cpp:class:`MarketByOrder <roq::MarketByOrderUpdate>`
        - |cross-mark|
        -
      * - :cpp:class:`TradeSummary <roq::TradeSummary>`
        - |check-mark|
        -
      * - :cpp:class:`Statistics <roq::StatisticsUpdate>`
        - |check-mark|
        -
      * - :cpp:class:`TimeSeries <roq::TimeSeriesUpdate>`
        - |check-mark|
        -

  .. grid-item-card::  Orders & Quotes

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:class:`CreateOrder <roq::CreateOrder>`
        - |check-mark|
        -
      * - :cpp:class:`ModifyOrder <roq::ModifyOrder>`
        - |check-mark|
        -
      * - :cpp:class:`CancelOrder <roq::CancelOrder>`
        - |check-mark|
        -
      * - :cpp:class:`CancelAllOrders <roq::CancelAllOrders>`
        - |check-mark|
        -
      * - :cpp:class:`MassQuote <roq::MassQuote>`
        - |negative-cross-mark|
        -
      * - :cpp:class:`CancelQuotes <roq::CancelQuotes>`
        - |negative-cross-mark|
        -

  .. grid-item-card::  Account

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:class:`Funds <roq::FundsUpdate>`
        - |check-mark|
        -
      * - :cpp:class:`Position <roq::PositionUpdate>`
        - |check-mark|
        -


.. note::

   |check-mark| = Available.

   |negative-cross-mark| = Not implemented.

   |cross-mark| = Unavailable.


Using
-----

.. code-block:: shell

   $ roq-deribit [FLAGS]


.. _roq-deribit-flags:

Flags
-----

.. code-block:: shell

   $ roq-deribit --help

.. tab:: Flags

   .. include:: flags/flags.rstinc

.. tab:: FIX

   .. include:: flags/fix.rstinc

.. tab:: REST

   .. include:: flags/rest.rstinc

.. tab:: WS

   .. include:: flags/ws.rstinc

.. tab:: Multicast

   .. include:: flags/multicast.rstinc

.. tab:: MBP

   .. include:: flags/mbp.rstinc

.. tab:: Request

   .. include:: flags/request.rstinc

.. tab:: Misc

   .. include:: flags/misc.rstinc


Environments
------------

.. tab:: Prod

   .. code-block:: shell

      $ --flagfile $CONDA_PREFIX/share/roq-deribit/flags/prod/flags.cfg

   .. include:: flags/prod/flags.cfg
     :code: ini

.. tab:: Test

   .. code-block:: shell

      $ --flagfile $CONDA_PREFIX/share/roq-deribit/flags/test/flags.cfg

   .. include:: flags/test/flags.cfg
     :code: ini


Configuration
-------------

.. code-block:: shell

   $ --config_file $CONDA_PREFIX/share/roq-deribit/config.toml

.. important::

   The template will be replaced when the software is upgraded.
   Make a copy and modify to your needs.

.. include:: config.toml
   :code: toml


Market Data
-----------

Inbound
~~~~~~~

.. tab:: TradingStatus

   .. list-table::
     :header-rows: 1
     :widths: auto
     :align: left

     * - :code:`state`
       -
       -

     * - :code:`closed`
       - |right-double-arrow|
       - :cpp:enumerator:`CLOSE <roq::TradingStatus::CLOSE>`

     * - :code:`open`
       - |right-double-arrow|
       - :cpp:enumerator:`OPEN <roq::TradingStatus::OPEN>`

     * - :code:`created`
       - |right-double-arrow|
       - :cpp:enumerator:`UNDEFINED <roq::TradingStatus::UNDEFINED>`

     * - :code:`settled`
       - |right-double-arrow|
       - :cpp:enumerator:`UNDEFINED <roq::TradingStatus::UNDEFINED>`

     * - :code:`terminated`
       - |right-double-arrow|
       - :cpp:enumerator:`UNDEFINED <roq::TradingStatus::UNDEFINED>`

     * - :code:`inactive`
       - |right-double-arrow|
       - :cpp:enumerator:`UNDEFINED <roq::TradingStatus::UNDEFINED>`

     * - :code:`deactivated`
       - |right-double-arrow|
       - :cpp:enumerator:`UNDEFINED <roq::TradingStatus::UNDEFINED>`

     * - :code:`started`
       - |right-double-arrow|
       - :cpp:enumerator:`UNDEFINED <roq::TradingStatus::UNDEFINED>`


.. tab:: StatisticsType

   .. list-table::
     :header-rows: 1
     :widths: auto
     :align: left

     * - Tag
       - Field
       - Value
       - Comment
       -
       -

     * - :code:`746`
       - :code:`OpenInterest`
       -
       -
       - |right-double-arrow|
       - :cpp:enumerator:`PRE_OPEN_INTEREST <roq::StatisticsType::PRE_OPEN_INTEREST>`

     * - :code:`100090`
       - :code:`MarkPrice`
       -
       -
       - |right-double-arrow|
       - :cpp:enumerator:`PRE_SETTLEMENT_PRICE <roq::StatisticsType::SETTLEMENT_PRICE>`

     * - :code:`269`
       - :code:`MDEntryType`
       - :code:`3`
       - Index value
       - |right-double-arrow|
       - :cpp:enumerator:`INDEX_VALUE <roq::StatisticsType::INDEX_VALUE>`

     * - :code:`269`
       - :code:`MDEntryType`
       - :code:`6`
       - Settlement price
       - |right-double-arrow|
       - :cpp:enumerator:`SETTLEMENT_PRICE <roq::StatisticsType::SETTLEMENT_PRICE>`


Order Management
----------------


Inbound
~~~~~~~

.. tab:: OrderType

   .. list-table::
     :header-rows: 1
     :widths: auto
     :align: left

     * - Enum
       -
       -

     * - :code:`MARKET`
       - |right-double-arrow|
       - :cpp:enumerator:`MARKET <roq::OrderType::MARKET>`

     * - :code:`LIMIT`
       - |right-double-arrow|
       - :cpp:enumerator:`LIMIT <roq::OrderType::LIMIT>`


.. tab:: TimeInForce

   .. list-table::
     :header-rows: 1
     :widths: auto
     :align: left

     * - Enum
       -
       -

     * - :code:`GTC`
       - |right-double-arrow|
       - :cpp:enumerator:`GTC <roq::TimeInForce::GTC>`

     * - :code:`IOC`
       - |right-double-arrow|
       - :cpp:enumerator:`IOC <roq::TimeInForce::IOC>`

     * - :code:`FOK`
       - |right-double-arrow|
       - :cpp:enumerator:`FOK <roq::TimeInForce::FOK>`

     * - :code:`GTX`
       - |right-double-arrow|
       - :cpp:enumerator:`GTX <roq::TimeInForce::GTX>`


.. tab:: OrderStatus

   .. list-table::
     :header-rows: 1
     :widths: auto
     :align: left

     * - Enum
       -
       -

     * - :code:`NEW`
       - |right-double-arrow|
       - :cpp:enumerator:`WORKING <roq::OrderStatus::WORKING>`

     * - :code:`PARTIALLY_FILLED`
       - |right-double-arrow|
       - :cpp:enumerator:`WORKING <roq::OrderStatus::WORKING>`

     * - :code:`FILLED`
       - |right-double-arrow|
       - :cpp:enumerator:`COMPLETED <roq::OrderStatus::COMPLETED>`

     * - :code:`CANCELED`
       - |right-double-arrow|
       - :cpp:enumerator:`CANCELED <roq::OrderStatus::CANCELED>`

     * - :code:`EXPIRED`
       - |right-double-arrow|
       - :cpp:enumerator:`EXPIRED <roq::OrderStatus::EXPIRED>`

     * - :code:`NEW_INSURANCE`
       - |right-double-arrow|
       - :cpp:enumerator:`UNDEFINED <roq::OrderStatus::UNDEFINED>`

     * - :code:`NEW_ADL`
       - |right-double-arrow|
       - :cpp:enumerator:`UNDEFINED <roq::OrderStatus::UNDEFINED>`




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


Outbound
~~~~~~~~

.. tab:: CreateOrder

   .. list-table::
     :header-rows: 1
     :widths: auto
     :align: left

     * - :cpp:member:`order_type <roq::CreateOrder::order_type>`
       - :cpp:member:`execution_instructions <roq::CreateOrder::execution_instructions>`
       - :cpp:member:`price <roq::CreateOrder::price>`
       - :cpp:member:`stop_price <roq::CreateOrder::stop_price>`
       -
       - :code:`type`
       - :code:`price`
       - :code:`stopPrice`
       - :code:`reduceOnly`

     * - :cpp:enumerator:`MARKET <roq::OrderType::MARKET>`
       -
       - :code:`NaN`
       - :code:`NaN`
       - |right-double-arrow|
       - :code:`MARKET`
       - |cross-mark|
       - |cross-mark|
       -

     * - :cpp:enumerator:`MARKET <roq::OrderType::MARKET>`
       -
       - :code:`NaN`
       - |check-mark|
       - |right-double-arrow|
       - :code:`MARKET`
       - |cross-mark|
       - |check-mark|
       -

     * - :cpp:enumerator:`LIMIT <roq::OrderType::LIMIT>`
       -
       - |check-mark|
       - :code:`NaN`
       - |right-double-arrow|
       - :code:`LIMIT`
       - |check-mark|
       - |cross-mark|
       -

     * - :cpp:enumerator:`LIMIT <roq::OrderType::LIMIT>`
       -
       - |check-mark|
       - |check-mark|
       - |right-double-arrow|
       - :code:`LIMIT`
       - |check-mark|
       - |check-mark|
       -


.. tab:: ModifyOrder

   TBD


.. tab:: CancelOrder

   TBD


.. tab:: CancelAllOrders

   TBD


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


Comments
--------

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

* The gateway must be restarted at least daily if you use the multicast feed.
  The reason is the snapshot vs events inconsistency mentioned under the constraints.
  The book effectively becomes more and more *wrong* for big market moves.

* The exchange doesn't notify us with the confirmed bar for time-series data.


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
       --multicast_config_file "--flagfile $CONDA_PREFIX/share/roq-deribit/prod/channels.json"

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


Exchange
~~~~~~~~

* `Website <https://www.deribit.com/>`__
* `Test <https://test.deribit.com/>`__
* `Status <https://deribit.statuspage.io/>`__
* `Telegram <https://t.me/s/deribit_notifications>`__
* `Support <mailto:support@deribit.com>`__
* `Technical Support <mailto:dev@deribit.com>`__
* `Documentation <https://docs.deribit.com/v2/>`__
* `Deribit New London Infrastructure <https://www.deribit.com/pages/information/Londonsetup>`__
