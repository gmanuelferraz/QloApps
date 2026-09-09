{*
* Auditor de Integridade & Sobreposição de Inventário (QloApps)
* Template Smarty: audit_dashboard.tpl
*}

<div class="panel">
    <div class="panel-heading">
        <i class="icon-calendar"></i> {l s='Auditor de Integridade & Conflitos de Ocupação' mod='qloinventoryaudit'}
    </div>

    {if isset($auditError) && $auditError}
        <div class="alert alert-danger">
            <i class="icon-warning-sign"></i> {$auditError|escape:'html':'UTF-8'}
        </div>
    {/if}

    {* Barra de Ações Rápidas: Carregamento do BD ou Fixture Demo *}
    <div class="well">
        <div class="row">
            <div class="col-md-7">
                <form method="post" action="{if isset($currentAction)}{$currentAction|escape:'html':'UTF-8'}{/if}" class="form-inline">
                    <label class="control-label" style="margin-right: 8px;">
                        <i class="icon-database"></i> {l s='Carregar do Banco de Dados:' mod='qloinventoryaudit'}
                    </label>
                    <div class="form-group">
                        <input type="date" name="db_date_from" value="{if isset($dbDateFrom)}{$dbDateFrom|escape:'html':'UTF-8'}{/if}" class="form-control input-sm" title="{l s='Data Inicial' mod='qloinventoryaudit'}" />
                    </div>
                    <div class="form-group" style="margin-left: 5px;">
                        <input type="date" name="db_date_to" value="{if isset($dbDateTo)}{$dbDateTo|escape:'html':'UTF-8'}{/if}" class="form-control input-sm" title="{l s='Data Final' mod='qloinventoryaudit'}" />
                    </div>
                    <button type="submit" name="submitLoadFromDb" class="btn btn-default btn-sm" style="margin-left: 5px;">
                        <i class="icon-download"></i> {l s='Buscar Reservas (Máx 200)' mod='qloinventoryaudit'}
                    </button>
                </form>
            </div>
            <div class="col-md-5 text-right">
                <button type="button" id="btnLoadDemoFixture" class="btn btn-info btn-sm">
                    <i class="icon-file-text"></i> {l s='Preencher com Fixture Demo (5 Reservas)' mod='qloinventoryaudit'}
                </button>
            </div>
        </div>
    </div>

    {* Container oculto com o JSON da fixture de demonstração *}
    <textarea id="demo_fixture_hidden" style="display:none;">{if isset($demoFixtureJson)}{$demoFixtureJson|escape:'html':'UTF-8'}{/if}</textarea>

    {* Formulário Principal de Submissão do Lote JSON *}
    <form method="post" action="{if isset($currentAction)}{$currentAction|escape:'html':'UTF-8'}{/if}" class="form-horizontal">
        <div class="form-group">
            <label class="control-label col-lg-3">
                <span class="label-tooltip" data-toggle="tooltip" title="{l s='Lote de reservas em formato JSON (máximo de 200 reservas).' mod='qloinventoryaudit'}">
                    {l s='Lote de Reservas (JSON):' mod='qloinventoryaudit'}
                </span>
            </label>
            <div class="col-lg-8">
                <textarea id="audit_batch_json" name="audit_batch_json" rows="8" class="form-control" placeholder='{literal}{"audit_batch_id": "lote-01", "reservations": [{"reservation_id": "RES-01", "room_id": "101", "check_in": "2026-09-01", "check_out": "2026-09-05", "guest_name": "Carlos"}]}{/literal}'>{if isset($rawJson)}{$rawJson|escape:'html':'UTF-8'}{/if}</textarea>
                <p class="help-block">
                    <i class="icon-info-sign"></i> {l s='O serviço local em C++ avalia colisões em intervalos semi-abertos [check_in, check_out) e calcula a severidade.' mod='qloinventoryaudit'}
                </p>
            </div>
        </div>

        <div class="form-group">
            <div class="col-lg-offset-3 col-lg-8">
                <button type="submit" name="submitRunAudit" class="btn btn-primary btn-lg">
                    <i class="icon-search"></i> {l s='Executar Auditoria de Sobreposição' mod='qloinventoryaudit'}
                </button>
            </div>
        </div>
    </form>

    {* Painel de Resultados Consolidados *}
    {if isset($auditResult) && $auditResult}
        <hr />
        <div class="panel" style="background-color: #fcfcfc;">
            <div class="panel-heading">
                <i class="icon-bar-chart"></i> {l s='Relatório Consolidado de Auditoria' mod='qloinventoryaudit'}
                {if isset($auditResult.audit_batch_id)}
                    <span class="badge" style="margin-left: 10px;">Batch ID: {$auditResult.audit_batch_id|escape:'html':'UTF-8'}</span>
                {/if}
            </div>

            <div class="row" style="margin: 15px 0;">
                <div class="col-md-4">
                    <div class="well text-center" style="margin-bottom: 0;">
                        <h4>{l s='Total de Reservas Auditadas' mod='qloinventoryaudit'}</h4>
                        <span class="badge" style="font-size: 20px; padding: 6px 12px; background-color: #337ab7;">
                            {$auditResult.total_reservations_audited|escape:'html':'UTF-8'}
                        </span>
                    </div>
                </div>
                <div class="col-md-4">
                    <div class="well text-center" style="margin-bottom: 0;">
                        <h4>{l s='Quartos Físicos Auditados' mod='qloinventoryaudit'}</h4>
                        <span class="badge" style="font-size: 20px; padding: 6px 12px; background-color: #5bc0de;">
                            {$auditResult.total_rooms_audited|escape:'html':'UTF-8'}
                        </span>
                    </div>
                </div>
                <div class="col-md-4">
                    <div class="well text-center" style="margin-bottom: 0;">
                        <h4>{l s='Status de Conflitos' mod='qloinventoryaudit'}</h4>
                        {if $auditResult.total_conflicts_found > 0}
                            <span class="label label-danger" style="font-size: 16px; padding: 8px 12px;">
                                <i class="icon-warning-sign"></i> {$auditResult.total_conflicts_found|escape:'html':'UTF-8'} {l s='CONFLITO(S) DETECTADO(S)' mod='qloinventoryaudit'}
                            </span>
                        {else}
                            <span class="label label-success" style="font-size: 16px; padding: 8px 12px;">
                                <i class="icon-check"></i> {l s='NENHUM CONFLITO DETECTADO' mod='qloinventoryaudit'}
                            </span>
                        {/if}
                    </div>
                </div>
            </div>

            {if $auditResult.total_conflicts_found > 0}
                <div class="clearfix" style="margin-bottom: 15px;">
                    <form method="post" action="{if isset($currentAction)}{$currentAction|escape:'html':'UTF-8'}{/if}" class="pull-right">
                        <input type="hidden" name="export_conflicts_json" value="{if isset($exportConflictsJson)}{$exportConflictsJson|escape:'html':'UTF-8'}{/if}" />
                        <button type="submit" name="submitExportCsv" class="btn btn-success">
                            <i class="icon-download"></i> {l s='Exportar Relatório em CSV' mod='qloinventoryaudit'}
                        </button>
                    </form>
                    <h4 style="margin-top: 10px;">
                        <i class="icon-list"></i> {l s='Detalhamento das Sobreposições:' mod='qloinventoryaudit'}
                    </h4>
                </div>

                <div class="table-responsive">
                    <table class="table table-bordered table-striped table-hover">
                        <thead>
                            <tr class="nodrag nodrop">
                                <th>{l s='Quarto Físico' mod='qloinventoryaudit'}</th>
                                <th>{l s='Reserva A' mod='qloinventoryaudit'}</th>
                                <th>{l s='Reserva B' mod='qloinventoryaudit'}</th>
                                <th>{l s='Período do Conflito' mod='qloinventoryaudit'}</th>
                                <th>{l s='Noites Sobrepostas' mod='qloinventoryaudit'}</th>
                                <th>{l s='Severidade' mod='qloinventoryaudit'}</th>
                                <th>{l s='Mensagem Operacional' mod='qloinventoryaudit'}</th>
                            </tr>
                        </thead>
                        <tbody>
                            {foreach from=$auditResult.conflicts item=conflict}
                                <tr>
                                    <td>
                                        <strong><i class="icon-tag"></i> {$conflict.room_id|escape:'html':'UTF-8'}</strong>
                                    </td>
                                    <td>
                                        <span class="label label-default">{$conflict.reservation_a_id|escape:'html':'UTF-8'}</span>
                                    </td>
                                    <td>
                                        <span class="label label-default">{$conflict.reservation_b_id|escape:'html':'UTF-8'}</span>
                                    </td>
                                    <td>
                                        <i class="icon-calendar"></i> {$conflict.overlap_start|escape:'html':'UTF-8'} {l s='até' mod='qloinventoryaudit'} {$conflict.overlap_end|escape:'html':'UTF-8'}
                                    </td>
                                    <td>
                                        <strong>{$conflict.overlap_nights|escape:'html':'UTF-8'}</strong> {l s='noite(s)' mod='qloinventoryaudit'}
                                    </td>
                                    <td>
                                        {if $conflict.severity == 'HIGH'}
                                            <span class="label label-danger">
                                                <i class="icon-exclamation-triangle"></i> {l s='ALTA' mod='qloinventoryaudit'}
                                            </span>
                                        {else}
                                            <span class="label label-warning">
                                                <i class="icon-warning-sign"></i> {l s='MÉDIA' mod='qloinventoryaudit'}
                                            </span>
                                        {/if}
                                    </td>
                                    <td>
                                        <small>{$conflict.message|escape:'html':'UTF-8'}</small>
                                    </td>
                                </tr>
                            {/foreach}
                        </tbody>
                    </table>
                </div>
            {/if}
        </div>
    {/if}
</div>

{literal}
<script type="text/javascript">
    document.addEventListener('DOMContentLoaded', function () {
        var btnDemo = document.getElementById('btnLoadDemoFixture');
        var txtBatch = document.getElementById('audit_batch_json');
        var hiddenFixture = document.getElementById('demo_fixture_hidden');

        if (btnDemo && txtBatch && hiddenFixture) {
            btnDemo.addEventListener('click', function () {
                txtBatch.value = hiddenFixture.value;
            });
        }
    });
</script>
{/literal}
