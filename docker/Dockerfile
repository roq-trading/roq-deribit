FROM condaforge/miniforge3

MAINTAINER Hans Erik Thrane <info@roq-trading.com>

RUN conda install --quiet --yes --channel https://roq-trading.com/conda/unstable roq-deribit

ENTRYPOINT [ "/opt/conda/bin/roq-deribit" ]

CMD [ "--help" ]
