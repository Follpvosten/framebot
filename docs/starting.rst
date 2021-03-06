Começando
=========

Framebot é uma biblioteca feita em C para facilitar o uso da BOT API do telegram.

Instalação
----------

Para instalar a biblioteca você deve primeiro clonar o nosso repositório:

.. code-block:: shell

   git clone https://github.com/giancarlopro/framebot

Agora você vai precisar ter o `cmake` instalado, e algumas libs:

.. code-block:: shell

   sudo apt install cmake openssl curl jansson

Após isso entre na pasta onde a biblioteca foi clonada, por padrão vai ser `framebot`:

.. code-block:: shell

   cd framebot

Agora crie uma pasta para compilar a biblioteca e rode o cmake para criar o makefile:

.. code-block:: shell

   mkdir build
   cd build
   cmake ..``

Agora utilize o makefile para compilar e instalar a biblioteca:

.. code-block:: shell

   make
   make install

Configuração
------------

Para utilizar a biblioteca em seus projetos, você deve informar ao compilador explicitamente
as bibliotecas que serão compiladas juntas.

* Se estiver compilando pelo terminal:

.. code-block:: shell

   gcc projeto.c -o out -lcurl -ljansson -lframebot

* Se estiver utilizando o cmake basta informar no seu CMakeLists.txt, em target_link_libraries:

.. code-block:: shell

   target_link_libraries(seuexecutavel framebot)

Utilização
----------

Para utilizar a biblioteca em seus projetos, você deve incluir o arquivo `framebot.h`.

.. code-block:: c

   #include <framebot/framebot.h>

Exemplos
--------

Você encontra exemplos de uso da biblioteca na pasta `examples` em nosso repositório no `GitHub`_.

.. _`GitHub`: https://github.com/giancarlopro/framebot/blob/master/examples